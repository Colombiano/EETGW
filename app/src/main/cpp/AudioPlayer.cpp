#include "AudioPlayer.h"
#include <android/log.h>
#include <cmath>
#include <cstring>

#define LOG_TAG "EETGW_Player"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace eetgw {

AudioPlayer::AudioPlayer() = default;
AudioPlayer::~AudioPlayer() { shutdown(); }

bool AudioPlayer::initialize(const AudioConfig& config) {
    config_ = config;
    pcmBuffer_.resize(config_.bufferSize * config_.channels);
    visualizerRing_.resize(config_.sampleRate * config_.channels);
    
    if (!setupStream(config)) {
        LOGE("Failed to setup audio stream");
        return false;
    }
    
    return true;
}

void AudioPlayer::shutdown() {
    stop();
    closeStream();
    pcmBuffer_.clear();
    visualizerRing_.clear();
    engine_ = nullptr;
}

bool AudioPlayer::isReady() const {
    return stream_ != nullptr;
}

bool AudioPlayer::play() {
    if (!stream_) return false;
    
    auto result = stream_->requestStart();
    if (result == oboe::Result::OK) {
        state_.store(PlaybackState::PLAYING);
        return true;
    }
    return false;
}

bool AudioPlayer::pause() {
    if (!stream_) return false;
    
    auto result = stream_->requestPause();
    if (result == oboe::Result::OK || result == oboe::Result::ErrorUnimplemented) {
        stream_->requestStop();
        state_.store(PlaybackState::PAUSED);
        return true;
    }
    return false;
}

bool AudioPlayer::stop() {
    if (!stream_) return false;
    
    auto result = stream_->requestStop();
    if (result == oboe::Result::OK) {
        state_.store(PlaybackState::STOPPED);
        {
            std::lock_guard<std::mutex> lock(pcmMutex_);
            memset(pcmBuffer_.data(), 0, pcmBuffer_.size() * sizeof(int16_t));
        }
        return true;
    }
    return false;
}

bool AudioPlayer::seekTo(int64_t positionMs) {
    if (!engine_) return false;
    
    bool wasPlaying = (state_.load() == PlaybackState::PLAYING);
    
    if (wasPlaying) {
        stream_->requestStop();
    }
    
    bool result = engine_->seekToMs(positionMs);
    
    if (result && wasPlaying) {
        stream_->requestStart();
    }
    
    return result;
}

void AudioPlayer::setVolume(float volume) {
    volume = std::max(0.0f, std::min(1.0f, volume));
    volume_.store(volume);
}

float AudioPlayer::getVolume() const {
    return volume_.load();
}

void AudioPlayer::volumeUp(float delta) {
    setVolume(getVolume() + delta);
}

void AudioPlayer::volumeDown(float delta) {
    setVolume(getVolume() - delta);
}

void AudioPlayer::setVolumeBoost(bool enabled) {
    volumeBoost_.store(enabled);
}

bool AudioPlayer::isVolumeBoost() const {
    return volumeBoost_.load();
}

PlaybackState AudioPlayer::getState() const {
    return state_.load();
}

int64_t AudioPlayer::getCurrentPositionMs() const {
    if (engine_) {
        return engine_->getCurrentPositionMs();
    }
    return 0;
}

int64_t AudioPlayer::getDurationMs() const {
    if (engine_) {
        return engine_->getDurationMs();
    }
    return 0;
}

double AudioPlayer::getLatencyMs() const {
    if (!stream_) return 0.0;
    
    auto result = stream_->calculateLatencyMillis();
    if (result) {
        return result.value();
    }
    return 0.0;
}

std::vector<float> AudioPlayer::getVisualizerData(size_t points) {
    std::lock_guard<std::mutex> lock(visualizerMutex_);
    
    std::vector<float> data(points, 0.0f);
    if (visualizerRing_.empty()) return data;
    
    size_t step = visualizerRing_.size() / (points * config_.channels);
    if (step == 0) step = 1;
    
    for (size_t i = 0; i < points; i++) {
        size_t idx = (visualizerWritePos_ + i * step * config_.channels) % visualizerRing_.size();
        
        float sample = 0;
        for (int ch = 0; ch < config_.channels; ch++) {
            sample += std::abs(visualizerRing_[(idx + ch) % visualizerRing_.size()]) / 32768.0f;
        }
        sample /= config_.channels;
        
        data[i] = std::min(sample * 2.0f, 1.0f);
    }
    
    return data;
}

oboe::DataCallbackResult AudioPlayer::onAudioReady(
        oboe::AudioStream* stream,
        void* audioData,
        int32_t numFrames) {
    
    auto* outputBuffer = static_cast<int16_t*>(audioData);
    
    int32_t framesGenerated = processAudio(outputBuffer, numFrames);
    
    if (framesGenerated < numFrames) {
        memset(outputBuffer + framesGenerated * config_.channels, 0,
               (numFrames - framesGenerated) * config_.channels * sizeof(int16_t));
        
        if (framesGenerated == 0 && state_.load() == PlaybackState::PLAYING) {
            state_.store(PlaybackState::STOPPED);
            if (progressCallback_) {
                progressCallback_(getDurationMs(), getDurationMs());
            }
        }
    }
    
    applyVolume(outputBuffer, numFrames);
    
    {
        std::lock_guard<std::mutex> lock(visualizerMutex_);
        for (int32_t i = 0; i < numFrames * config_.channels; i++) {
            visualizerRing_[visualizerWritePos_] = outputBuffer[i];
            visualizerWritePos_ = (visualizerWritePos_ + 1) % visualizerRing_.size();
        }
    }
    
    int64_t posMs = getCurrentPositionMs();
    if (progressCallback_ && std::abs(posMs - lastProgressMs_) > 500) {
        lastProgressMs_ = posMs;
        progressCallback_(posMs, getDurationMs());
    }
    
    return oboe::DataCallbackResult::Continue;
}

int32_t AudioPlayer::processAudio(int16_t* outputBuffer, int32_t numFrames) {
    if (!engine_ || state_.load() != PlaybackState::PLAYING) {
        return 0;
    }
    
    int32_t totalFrames = 0;
    
    while (totalFrames < numFrames) {
        int framesNeeded = numFrames - totalFrames;
        
        int decoded = engine_->decodeNextFrame(
            outputBuffer + totalFrames * config_.channels, 
            framesNeeded
        );
        
        if (decoded <= 0) {
            break;
        }
        
        totalFrames += decoded;
    }
    
    return totalFrames;
}

void AudioPlayer::applyVolume(int16_t* buffer, int32_t frames) {
    float vol = volume_.load();
    if (volumeBoost_.load()) {
        vol *= VOLUME_BOOST_GAIN;
    }
    
    if (vol >= 0.999f && vol <= 1.001f) return;
    
    int32_t samples = frames * config_.channels;
    for (int32_t i = 0; i < samples; i++) {
        float sample = buffer[i] * vol;
        sample = std::max(-32768.0f, std::min(32767.0f, sample));
        buffer[i] = static_cast<int16_t>(sample);
    }
}

bool AudioPlayer::setupStream(const AudioConfig& config) {
    oboe::AudioStreamBuilder builder;
    
    builder.setDirection(oboe::Direction::Output)
           ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
           ->setSharingMode(oboe::SharingMode::Exclusive)
           ->setSampleRate(config.sampleRate)
           ->setChannelCount(config.channels)
           ->setFormat(oboe::AudioFormat::I16)
           ->setDataCallback(this)
           ->setUsage(oboe::Usage::Media)
           ->setContentType(oboe::ContentType::Music);
    
    builder.setAudioApi(oboe::AudioApi::AAudio);
    
    auto result = builder.openStream(stream_);
    
    if (result != oboe::Result::OK) {
        stream_.reset();
        builder.setAudioApi(oboe::AudioApi::OpenSLES);
        result = builder.openStream(stream_);
    }
    
    if (result != oboe::Result::OK) {
        return false;
    }
    
    return true;
}

void AudioPlayer::closeStream() {
    if (stream_) {
        stream_->close();
        stream_.reset();
    }
}

} // namespace eetgw
