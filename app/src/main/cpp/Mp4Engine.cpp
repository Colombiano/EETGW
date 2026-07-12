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
// Usa API MP4D_ (UPPERCASE) da versao do minimp4 instalada
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

    auto* demux = new MP4D_demux_t;
    memset(demux, 0, sizeof(MP4D_demux_t));
    mp4Context_->demuxState = demux;

    std::vector<uint8_t> header(std::min(fileSize, int64_t(65536)));
    size_t headerRead = fread(header.data(), 1, header.size(), mp4Context_->fileHandle);

    unsigned char* p = header.data();
    size_t bytes_left = headerRead;

    if (!MP4D_open(demux, &p, &bytes_left, 0)) {
        LOGE("MP4D_open failed: %s", path.c_str());
        mp4Context_.reset();
        return false;
    }

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

    // Extrai AudioSpecificConfig (ASC) do sample entry
    std::vector<uint8_t> asc;
    if (track.sample_entry.size > 0) {
        const uint8_t* se = track.sample_entry.data;
        size_t seSize = track.sample_entry.size;

        for (size_t i = 0; i + 2 < seSize; i++) {
            if (se[i] == 0x05 && se[i+1] > 0) {
                size_t ascLen = se[i+1];
                if (i + 2 + ascLen <= seSize) {
                    asc.assign(se + i + 2, se + i + 2 + ascLen);
                    break;
                }
            }
        }
    }

    if (asc.empty()) {
        LOGE("No ASC found in MP4: %s", path.c_str());
        mp4Context_.reset();
        return false;
    }

    aacDecoder_ = std::make_unique<AacDecoder>();
    if (!aacDecoder_->initialize(asc.data(), asc.size())) {
        LOGE("AAC decoder init failed: %s", path.c_str());
        mp4Context_.reset();
        return false;
    }

    unsigned sampleCount = track.sample_count;
    unsigned timescale = track.timescale > 0 ? track.timescale : 44100;

    const size_t CHUNK_SIZE = 64 * 1024;
    int64_t currentOffset = 0;

    for (unsigned i = 0; i < sampleCount; i++) {
        unsigned char* sampleData = nullptr;
        unsigned int sampleBytes = 0;
        unsigned int sampleTimestamp = 0;
        unsigned int sampleDuration = 0;

        if (MP4D_read_sample(demux, audioTrack, i, &sampleData, &sampleBytes,
                              &sampleTimestamp, &sampleDuration)) {
            AudioChunk chunk;
            chunk.offset = currentOffset;
            chunk.size = sampleBytes;
            chunk.sampleIndex = i;
            chunk.durationMs = (sampleDuration * 1000ULL) / timescale;
            mp4Context_->chunks.push_back(chunk);
            currentOffset += sampleBytes;
        }
    }

    if (track.duration > 0 && timescale > 0) {
        metadata_.durationMs = (track.duration * 1000ULL) / timescale;
    } else {
        metadata_.durationMs = (sampleCount * 1024LL * 1000) / timescale;
    }

    metadata_.sampleRate = aacDecoder_->getSampleRate();
    metadata_.channels = aacDecoder_->getChannels();
    metadata_.hasVideo = true;

    mp4Context_->chunkBuffer.resize(CHUNK_SIZE * 2);
    currentChunkIndex_ = 0;
    loaded_ = true;

    return true;
}

int Mp3Engine::decodeMp4Frame(int16_t* buffer, int maxFrames) {
    if (!mp4Context_ || !aacDecoder_ || !mp4Context_->demuxState) return 0;

    if (currentChunkIndex_ >= mp4Context_->chunks.size()) {
        return 0;
    }

    const auto& chunk = mp4Context_->chunks[currentChunkIndex_];

    if (fseek(mp4Context_->fileHandle, chunk.offset, SEEK_SET) != 0) {
        return 0;
    }

    std::vector<uint8_t> aacData(chunk.size);
    size_t read = fread(aacData.data(), 1, chunk.size, mp4Context_->fileHandle);

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
