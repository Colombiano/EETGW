#include "Mp3Engine.h"
#include "MetadataCache.h"
#include <android/log.h>
#include <cstring>
#include <cmath>

// minimp4 single-header (apenas MP4 / only MP4)
#define MINIMP4_IMPLEMENTATION
#include "minimp4/minimp4.h"

// fdk-aac decoder
#include <fdk-aac/aacdecoder_lib.h>

#define LOG_TAG "EETGW_MP4"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace eetgw {

// =============================================================================
// AacDecoder — Constructor / Destructor
// =============================================================================

AacDecoder::AacDecoder() : handle_(nullptr), sampleRate_(44100), channels_(2), initialized_(false) {
}

AacDecoder::~AacDecoder() {
    cleanup();
}

// =============================================================================
// Callback de leitura para MP4D_open / Read callback for MP4D_open
// =============================================================================

static int mp4ReadCallback(int64_t offset, void* buffer, size_t size, void* token) {
    FILE* f = static_cast<FILE*>(token);
    if (fseek(f, offset, SEEK_SET) != 0) {
        return 1;
    }
    size_t read = fread(buffer, 1, size, f);
    return (read == size) ? 0 : 1;
}

// =============================================================================
// AacDecoder — Implementacao completa / Full implementation
// =============================================================================

bool AacDecoder::initialize(const uint8_t* ascData, size_t ascSize) {
    cleanup();

    handle_ = aacDecoder_Open(TT_MP4_RAW, 1);
    if (!handle_) {
        LOGE("aacDecoder_Open failed");
        return false;
    }

    UCHAR* asc = const_cast<UCHAR*>(ascData);
    UINT ascLen = static_cast<UINT>(ascSize);
    AAC_DECODER_ERROR err = aacDecoder_ConfigRaw(static_cast<HANDLE_AACDECODER>(handle_), &asc, &ascLen);

    if (err != AAC_DEC_OK) {
        LOGE("aacDecoder_ConfigRaw failed: 0x%04x", err);
        cleanup();
        return false;
    }

    CStreamInfo* info = aacDecoder_GetStreamInfo(static_cast<HANDLE_AACDECODER>(handle_));
    if (info) {
        sampleRate_ = info->sampleRate;
        channels_ = info->numChannels;
    }

    initialized_ = true;
    return true;
}

int AacDecoder::decode(const uint8_t* aacData, size_t aacSize, int16_t* pcmBuffer, int maxFrames) {
    if (!handle_ || !initialized_) return 0;

    HANDLE_AACDECODER hDec = static_cast<HANDLE_AACDECODER>(handle_);

    UCHAR* inBuffer = const_cast<UCHAR*>(aacData);
    UINT inSize = static_cast<UINT>(aacSize);
    UINT validBytes = inSize;

    AAC_DECODER_ERROR err = aacDecoder_Fill(hDec, &inBuffer, &inSize, &validBytes);
    if (err != AAC_DEC_OK) {
        return 0;
    }

    INT_PCM* outBuffer = reinterpret_cast<INT_PCM*>(pcmBuffer);
    int outSize = maxFrames * channels_;

    err = aacDecoder_DecodeFrame(hDec, outBuffer, outSize, 0);

    if (err == AAC_DEC_NOT_ENOUGH_BITS) return 0;
    if (err != AAC_DEC_OK) return 0;

    CStreamInfo* info = aacDecoder_GetStreamInfo(hDec);
    if (info) {
        return info->frameSize;
    }

    return 0;
}

void AacDecoder::cleanup() {
    if (handle_) {
        aacDecoder_Close(static_cast<HANDLE_AACDECODER>(handle_));
        handle_ = nullptr;
    }
    initialized_ = false;
}

// =============================================================================
// MP4 Implementation (minimp4 + fdk-aac)
// Versao simplificada para compatibilidade / Simplified for compatibility
// =============================================================================

bool Mp3Engine::loadMp4(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);

    unload();

    metadata_.path = path;
    metadata_.format = AudioFormat::MP4_AAC;

    mp4Context_ = std::make_unique<Mp4Context>();

    mp4Context_->fileHandle = fopen(path.c_str(), "rb");
    if (!mp4Context_->fileHandle) {
        LOGE("Cannot open MP4: %s", path.c_str());
        mp4Context_.reset();
        return false;
    }

    fseek(mp4Context_->fileHandle, 0, SEEK_END);
    int64_t fileSize = ftell(mp4Context_->fileHandle);
    fseek(mp4Context_->fileHandle, 0, SEEK_SET);

    if (fileSize <= 0) {
        LOGE("Empty MP4 file: %s", path.c_str());
        mp4Context_.reset();
        return false;
    }

    auto* demux = new MP4D_demux_t;
    memset(demux, 0, sizeof(MP4D_demux_t));
    mp4Context_->demuxState = demux;

    // Abre com callback de leitura / Open with read callback
    if (!MP4D_open(demux, mp4ReadCallback, mp4Context_->fileHandle, fileSize)) {
        LOGE("MP4D_open failed: %s", path.c_str());
        mp4Context_.reset();
        return false;
    }

    // Encontra track de audio / Find audio track
    int audioTrack = -1;
    for (unsigned i = 0; i < demux->track_count; i++) {
        if (demux->track[i].handler_type == MP4D_HANDLER_TYPE_SOUN) {
            audioTrack = i;
            break;
        }
    }

    if (audioTrack < 0) {
        LOGE("No audio track in MP4: %s", path.c_str());
        mp4Context_.reset();
        return false;
    }

    MP4D_track_t& track = demux->track[audioTrack];

    // Inicializa AAC decoder com ASC generico / Init with generic ASC
    // AAC-LC, 44100Hz, 2 channels = 0x12 0x10
    std::vector<uint8_t> asc;
    asc.push_back(0x12);
    asc.push_back(0x10);

    aacDecoder_ = std::make_unique<AacDecoder>();
    if (!aacDecoder_->initialize(asc.data(), asc.size())) {
        LOGE("AAC decoder init failed: %s", path.c_str());
        mp4Context_.reset();
        return false;
    }

    unsigned sampleCount = track.sample_count;
    unsigned timescale = track.timescale > 0 ? track.timescale : 44100;

    // Constroi chunks com offsets fixos / Build chunks with fixed offsets
    // Cada chunk = ~1 segundo de audio / Each chunk = ~1 second of audio
    const size_t CHUNK_SIZE = 16384;  // 16KB chunks
    int64_t filePos = 0;
    size_t numChunks = (fileSize + CHUNK_SIZE - 1) / CHUNK_SIZE;
    const size_t MAX_CHUNKS = 50000;

    for (size_t i = 0; i < std::min(numChunks, MAX_CHUNKS); i++) {
        AudioChunk chunk;
        chunk.offset = filePos;
        chunk.size = std::min((int64_t)CHUNK_SIZE, fileSize - filePos);
        chunk.sampleIndex = i;
        chunk.durationMs = (chunk.size * 1000ULL) / (128 * 1024 / 8);  // Estimativa @ 128kbps
        mp4Context_->chunks.push_back(chunk);
        filePos += chunk.size;
    }

    // Calcula duracao total / Calculate total duration
    if (track.duration && timescale > 0) {
        metadata_.durationMs = (*track.duration * 1000ULL) / timescale;
    } else {
        metadata_.durationMs = (sampleCount * 1024LL * 1000) / timescale;
    }

    metadata_.sampleRate = aacDecoder_->getSampleRate();
    metadata_.channels = aacDecoder_->getChannels();
    metadata_.hasVideo = true;

    mp4Context_->chunkBuffer.resize(CHUNK_SIZE * 2);
    currentChunkIndex_ = 0;
    loaded_ = true;

    LOGI("MP4 loaded: %s, chunks=%zu, duration=%ldms",
         path.c_str(), mp4Context_->chunks.size(), metadata_.durationMs);

    return true;
}

int Mp3Engine::decodeMp4Frame(int16_t* buffer, int maxFrames) {
    if (!mp4Context_ || !aacDecoder_ || !mp4Context_->demuxState) return 0;

    if (currentChunkIndex_ >= mp4Context_->chunks.size()) {
        return 0;
    }

    const auto& chunk = mp4Context_->chunks[currentChunkIndex_];

    if (!mp4Context_->fileHandle || fseek(mp4Context_->fileHandle, chunk.offset, SEEK_SET) != 0) {
        return 0;
    }

    // Le bloco do arquivo / Read block from file
    std::vector<uint8_t> aacData(chunk.size);
    size_t read = fread(aacData.data(), 1, chunk.size, mp4Context_->fileHandle);
    if (read == 0) {
        currentChunkIndex_++;
        return 0;
    }

    // Tenta decodificar / Try to decode
    int decoded = aacDecoder_->decode(aacData.data(), read, buffer, maxFrames);

    currentChunkIndex_++;

    if (decoded > 0 && metadata_.sampleRate > 0) {
        currentPositionMs_ += (decoded * 1000LL) / metadata_.sampleRate;
    }

    return decoded;
}

bool Mp3Engine::seekMp4(float ratio) {
    if (!mp4Context_ || mp4Context_->chunks.empty()) return false;

    ratio = std::max(0.0f, std::min(1.0f, ratio));

    currentChunkIndex_ = static_cast<size_t>(ratio * mp4Context_->chunks.size());
    currentChunkIndex_ = std::min(currentChunkIndex_, mp4Context_->chunks.size() - 1);

    currentPositionMs_ = static_cast<int64_t>(ratio * metadata_.durationMs);
    return true;
}

} // namespace eetgw