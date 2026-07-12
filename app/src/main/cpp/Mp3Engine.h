#pragma once

#include "DataTypes.h"
#include <memory>
#include <vector>
#include <mutex>
#include <string>

struct mp3dec_t;
struct mp3dec_frame_info;

namespace eetgw {

class AacDecoder;

// =============================================================================
// EETGW — Essa e pra tocar no Galaxy Watch
// Mp3Engine.h — Motor de decodificacao principal / Main decoding engine
// =============================================================================

class Mp3Engine {
public:
    Mp3Engine();
    ~Mp3Engine();

    bool loadFile(const std::string& path);
    void unload();
    bool isLoaded() const;

    int decodeNextFrame(int16_t* buffer, int maxFrames);
    bool seekToRatio(float ratio);
    bool seekToMs(int64_t positionMs);

    const Mp3Metadata& getMetadata() const { return metadata_; }
    int64_t getDurationMs() const { return metadata_.durationMs; }
    int64_t getCurrentPositionMs() const { return currentPositionMs_; }
    AudioFormat getFormat() const { return metadata_.format; }

    AudioFormat detectFormat(const std::string& path);

private:
    // MP3 / minimp3
    bool loadMp3(const std::string& path);
    int decodeMp3Frame(int16_t* buffer, int maxFrames);
    bool seekMp3(float ratio);
    
    std::unique_ptr<mp3dec_t> mp3Decoder_;
    std::vector<uint8_t> mp3FileData_;
    std::vector<uint8_t> mp3Remainder_;
    size_t mp3ReadPos_ = 0;
    int mp3Channels_ = 2;
    int mp3SampleRate_ = 44100;

    // MP4 / minimp4 + fdk-aac
    bool loadMp4(const std::string& path);
    int decodeMp4Frame(int16_t* buffer, int maxFrames);
    bool seekMp4(float ratio);
    
    std::unique_ptr<Mp4Context> mp4Context_;
    std::unique_ptr<AacDecoder> aacDecoder_;
    size_t currentChunkIndex_ = 0;

    // Estado geral / General state
    Mp3Metadata metadata_;
    int64_t currentPositionMs_ = 0;
    bool loaded_ = false;
    mutable std::mutex mutex_;
};

// =============================================================================
// AacDecoder — Wrapper para fdk-aac / Wrapper for fdk-aac
// =============================================================================
class AacDecoder {
public:
    AacDecoder();
    ~AacDecoder();

    bool initialize(const uint8_t* ascData, size_t ascSize);
    int decode(const uint8_t* aacData, size_t aacSize, int16_t* pcmBuffer, int maxFrames);
    
    int getSampleRate() const { return sampleRate_; }
    int getChannels() const { return channels_; }

    void cleanup();

private:
    void* handle_ = nullptr;
    int sampleRate_ = 44100;
    int channels_ = 2;
    bool initialized_ = false;
};

} // namespace eetgw
