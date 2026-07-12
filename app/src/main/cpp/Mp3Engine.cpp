#include "Mp3Engine.h"
#include "MetadataCache.h"
#include <android/log.h>
#include <fstream>
#include <cstring>
#include <cmath>

// minimp3 single-header (apenas MP3 / only MP3)
#define MINIMP3_IMPLEMENTATION
#include "minimp3/minimp3.h"
#include "minimp3/minimp3_ex.h"

#define LOG_TAG "EETGW_Engine"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace eetgw {

// —————————————————————————————————————————————————————————————————————————————
// Helpers para mp3dec_t via void* / Helpers for mp3dec_t through void*
// —————————————————————————————————————————————————————————————————————————————

static void deleteMp3Decoder(void* ptr) {
    delete static_cast<mp3dec_t*>(ptr);
}

static mp3dec_t* getMp3Decoder(const std::unique_ptr<void, void(*)(void*)>& ptr) {
    return static_cast<mp3dec_t*>(ptr.get());
}

// —————————————————————————————————————————————————————————————————————————————
// AacDecoder stubs (implementacao completa em Mp4Engine.cpp)
// —————————————————————————————————————————————————————————————————————————————

AacDecoder::AacDecoder() = default;
AacDecoder::~AacDecoder() { cleanup(); }

bool AacDecoder::initialize(const uint8_t* /*ascData*/, size_t /*ascSize*/) {
    return false;
}

int AacDecoder::decode(const uint8_t* /*aacData*/, size_t /*aacSize*/,
                       int16_t* /*pcmBuffer*/, int /*maxFrames*/) {
    return 0;
}

void AacDecoder::cleanup() {
    handle_ = nullptr;
    initialized_ = false;
}

// —————————————————————————————————————————————————————————————————————————————
// Helpers
// —————————————————————————————————————————————————————————————————————————————

static std::vector<uint8_t> readFile(const std::string& path) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) {
        LOGE("Cannot open file: %s", path.c_str());
        return {};
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size <= 0) {
        fclose(f);
        return {};
    }

    std::vector<uint8_t> data(size);
    size_t read = fread(data.data(), 1, size, f);
    (void)read;
    fclose(f);

    return data;
}

static std::string extractFilename(const std::string& path) {
    size_t lastSlash = path.find_last_of("/\\");
    std::string filename = (lastSlash != std::string::npos) ? path.substr(lastSlash + 1) : path;
    size_t lastDot = filename.find_last_of('.');
    if (lastDot != std::string::npos) {
        filename = filename.substr(0, lastDot);
    }
    return filename;
}

// —————————————————————————————————————————————————————————————————————————————
// Mp3Engine
// —————————————————————————————————————————————————————————————————————————————

Mp3Engine::Mp3Engine() : mp3Decoder_(nullptr, deleteMp3Decoder) {}
Mp3Engine::~Mp3Engine() { unload(); }

AudioFormat Mp3Engine::detectFormat(const std::string& path) {
    size_t dotPos = path.find_last_of('.');
    if (dotPos != std::string::npos) {
        std::string ext = path.substr(dotPos + 1);
        for (auto& c : ext) c = static_cast<char>(tolower(c));

        if (ext == "mp3") return AudioFormat::MP3;
        if (ext == "mp4" || ext == "m4a" || ext == "aac") return AudioFormat::MP4_AAC;
    }

    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return AudioFormat::UNKNOWN;

    uint8_t magic[12];
    size_t read = fread(magic, 1, 12, f);
    fclose(f);

    if (read >= 3 && magic[0] == 0xFF && (magic[1] & 0xE0) == 0xE0) {
        return AudioFormat::MP3;
    }
    if (read >= 8 && memcmp(magic + 4, "ftyp", 4) == 0) {
        return AudioFormat::MP4_AAC;
    }
    if (read >= 4 && memcmp(magic, "ID3", 3) == 0) {
        return AudioFormat::MP3;
    }

    return AudioFormat::UNKNOWN;
}

bool Mp3Engine::loadFile(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);

    Mp3Metadata cached;
    if (MetadataCache::instance().get(path, cached)) {
        metadata_ = cached;
        loaded_ = true;
        return true;
    }

    unload();

    metadata_.path = path;
    metadata_.format = detectFormat(path);

    bool success = false;
    switch (metadata_.format) {
        case AudioFormat::MP3:
            success = loadMp3(path);
            break;
        case AudioFormat::MP4_AAC:
        case AudioFormat::MP4_MP3:
            success = loadMp4(path);
            break;
        default:
            LOGE("Unknown format: %s", path.c_str());
            return false;
    }

    if (success) {
        if (metadata_.title.empty()) {
            metadata_.title = extractFilename(path);
        }
        MetadataCache::instance().put(path, metadata_);
        loaded_ = true;
    }

    return success;
}

void Mp3Engine::unload() {
    std::lock_guard<std::mutex> lock(mutex_);

    mp3FileData_.clear();
    mp3Remainder_.clear();
    mp3ReadPos_ = 0;
    mp3Decoder_.reset();

    aacDecoder_.reset();
    mp4Context_.reset();
    currentChunkIndex_ = 0;

    metadata_ = Mp3Metadata{};
    currentPositionMs_ = 0;
    loaded_ = false;
}

bool Mp3Engine::isLoaded() const {
    return loaded_;
}

int Mp3Engine::decodeNextFrame(int16_t* buffer, int maxFrames) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!loaded_) return 0;

    int decoded = 0;
    switch (metadata_.format) {
        case AudioFormat::MP3:
            decoded = decodeMp3Frame(buffer, maxFrames);
            break;
        case AudioFormat::MP4_AAC:
        case AudioFormat::MP4_MP3:
            decoded = decodeMp4Frame(buffer, maxFrames);
            break;
        default:
            break;
    }

    if (decoded > 0 && metadata_.sampleRate > 0) {
        currentPositionMs_ += (decoded * 1000LL) / metadata_.sampleRate;
    }

    return decoded;
}

bool Mp3Engine::seekToRatio(float ratio) {
    std::lock_guard<std::mutex> lock(mutex_);

    ratio = std::max(0.0f, std::min(1.0f, ratio));

    switch (metadata_.format) {
        case AudioFormat::MP3:
            return seekMp3(ratio);
        case AudioFormat::MP4_AAC:
        case AudioFormat::MP4_MP3:
            return seekMp4(ratio);
        default:
            return false;
    }
}

bool Mp3Engine::seekToMs(int64_t positionMs) {
    if (metadata_.durationMs <= 0) return false;
    float ratio = static_cast<float>(positionMs) / metadata_.durationMs;
    return seekToRatio(ratio);
}

// =============================================================================
// MP3 Implementation (minimp3)
// =============================================================================

bool Mp3Engine::loadMp3(const std::string& path) {
    mp3FileData_ = readFile(path);
    if (mp3FileData_.empty()) {
        LOGE("Failed to read MP3: %s", path.c_str());
        return false;
    }

    mp3Decoder_.reset(new mp3dec_t());
    mp3dec_init(getMp3Decoder(mp3Decoder_));

    mp3dec_file_info_t info;
    int ret = mp3dec_detect(mp3FileData_.data(), mp3FileData_.size(), &info);

    if (ret == 0 && info.hz > 0) {
        metadata_.sampleRate = info.hz;
        metadata_.channels = info.channels;
        metadata_.durationMs = info.avg_bitrate_kbps > 0
            ? (static_cast<int64_t>(mp3FileData_.size()) * 8) / (info.avg_bitrate_kbps)
            : 0;
        metadata_.bitrate = info.avg_bitrate_kbps * 1000;
    } else {
        metadata_.sampleRate = 44100;
        metadata_.channels = 2;

        mp3dec_frame_info_t frameInfo;
        int16_t framePCM[MINIMP3_MAX_SAMPLES_PER_FRAME];
        mp3dec_decode_frame(getMp3Decoder(mp3Decoder_), mp3FileData_.data(),
                            std::min(mp3FileData_.size(), size_t(4096)),
                            framePCM, &frameInfo);
        if (frameInfo.hz > 0) {
            metadata_.sampleRate = frameInfo.hz;
            metadata_.channels = frameInfo.channels;
        }
    }

    mp3ReadPos_ = 0;
    mp3Channels_ = metadata_.channels;
    mp3SampleRate_ = metadata_.sampleRate;

    return true;
}

int Mp3Engine::decodeMp3Frame(int16_t* buffer, int maxFrames) {
    if (!mp3Decoder_ || mp3FileData_.empty()) return 0;

    mp3dec_frame_info_t frameInfo;
    int totalFrames = 0;

    std::vector<uint8_t> input;
    input.insert(input.end(), mp3Remainder_.begin(), mp3Remainder_.end());

    if (mp3ReadPos_ < mp3FileData_.size()) {
        size_t toRead = std::min(mp3FileData_.size() - mp3ReadPos_, size_t(16384));
        input.insert(input.end(), mp3FileData_.begin() + mp3ReadPos_,
                     mp3FileData_.begin() + mp3ReadPos_ + toRead);
        mp3ReadPos_ += toRead;
    }

    if (input.empty()) return 0;

    size_t decodePos = 0;
    while (totalFrames < maxFrames && decodePos < input.size()) {
        int16_t pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];
        int samples = mp3dec_decode_frame(getMp3Decoder(mp3Decoder_),
                                          input.data() + decodePos,
                                          input.size() - decodePos,
                                          pcm, &frameInfo);

        if (frameInfo.frame_bytes == 0) break;
        decodePos += frameInfo.frame_bytes;

        if (samples > 0) {
            int framesToCopy = std::min(samples / frameInfo.channels, maxFrames - totalFrames);
            memcpy(buffer + totalFrames * frameInfo.channels, pcm,
                   framesToCopy * frameInfo.channels * sizeof(int16_t));
            totalFrames += framesToCopy;

            mp3SampleRate_ = frameInfo.hz;
            mp3Channels_ = frameInfo.channels;
        }
    }

    if (decodePos < input.size()) {
        mp3Remainder_.assign(input.begin() + decodePos, input.end());
    } else {
        mp3Remainder_.clear();
    }

    return totalFrames;
}

bool Mp3Engine::seekMp3(float ratio) {
    if (mp3FileData_.empty()) return false;

    mp3ReadPos_ = static_cast<size_t>(ratio * mp3FileData_.size());
    mp3Remainder_.clear();

    while (mp3ReadPos_ < mp3FileData_.size() - 1) {
        if (mp3FileData_[mp3ReadPos_] == 0xFF && (mp3FileData_[mp3ReadPos_ + 1] & 0xE0) == 0xE0) {
            break;
        }
        mp3ReadPos_++;
    }

    currentPositionMs_ = static_cast<int64_t>(ratio * metadata_.durationMs);
    return true;
}

// =============================================================================
// MP4 Stubs (implementacao real em Mp4Engine.cpp)
// =============================================================================

bool Mp3Engine::loadMp4(const std::string& /*path*/) {
    LOGE("MP4: loadMp4() should not be called from Mp3Engine.cpp");
    return false;
}

int Mp3Engine::decodeMp4Frame(int16_t* /*buffer*/, int /*maxFrames*/) {
    return 0;
}

bool Mp3Engine::seekMp4(float /*ratio*/) {
    return false;
}

} // namespace eetgw
