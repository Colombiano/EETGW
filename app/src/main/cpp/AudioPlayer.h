#pragma once

#include "DataTypes.h"
#include "Mp3Engine.h"
#include <oboe/Oboe.h>
#include <memory>
#include <atomic>
#include <mutex>
#include <vector>
#include <functional>

// =============================================================================
// EETGW — Essa e pra tocar no Galaxy Watch
// AudioPlayer.h — Reproducao de audio via Oboe / Audio playback via Oboe
// =============================================================================

namespace eetgw {

class AudioPlayer : public oboe::AudioStreamDataCallback {
public:
    AudioPlayer();
    ~AudioPlayer();

    bool initialize(const AudioConfig& config = AudioConfig{});
    void shutdown();
    bool isReady() const;

    bool play();
    bool pause();
    bool stop();
    bool seekTo(int64_t positionMs);

    void setVolume(float volume);
    float getVolume() const;
    void volumeUp(float delta = 0.05f);
    void volumeDown(float delta = 0.05f);
    void setVolumeBoost(bool enabled);
    bool isVolumeBoost() const;

    void setEngine(Mp3Engine* engine) { engine_ = engine; }
    Mp3Engine* getEngine() const { return engine_; }

    PlaybackState getState() const;
    int64_t getCurrentPositionMs() const;
    int64_t getDurationMs() const;
    double getLatencyMs() const;

    void setProgressCallback(std::function<void(int64_t, int64_t)> cb) {
        progressCallback_ = cb;
    }

    std::vector<float> getVisualizerData(size_t points);

    oboe::DataCallbackResult onAudioReady(
        oboe::AudioStream* stream,
        void* audioData,
        int32_t numFrames) override;

private:
    bool setupStream(const AudioConfig& config);
    void closeStream();

    int32_t processAudio(int16_t* outputBuffer, int32_t numFrames);
    void applyVolume(int16_t* buffer, int32_t frames);

    std::shared_ptr<oboe::AudioStream> stream_;
    AudioConfig config_;
    
    Mp3Engine* engine_ = nullptr;
    std::atomic<PlaybackState> state_{PlaybackState::STOPPED};
    
    std::atomic<float> volume_{1.0f};
    std::atomic<bool> volumeBoost_{false};
    static constexpr float VOLUME_BOOST_GAIN = 1.5f;
    
    std::vector<int16_t> pcmBuffer_;
    std::mutex pcmMutex_;
    
    std::vector<int16_t> visualizerRing_;
    size_t visualizerWritePos_ = 0;
    std::mutex visualizerMutex_;
    
    std::function<void(int64_t, int64_t)> progressCallback_;
    int64_t lastProgressMs_ = 0;
    
    std::atomic<int32_t> underflowCount_{0};
};

} // namespace eetgw