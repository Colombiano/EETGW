#include "MetadataCache.h"
#include <android/log.h>

#define LOG_TAG "EETGW_Cache"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

namespace eetgw {

MetadataCache& MetadataCache::instance() {
    static MetadataCache instance;
    return instance;
}

bool MetadataCache::get(const std::string& path, Mp3Metadata& out) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = cache_.find(path);
    if (it == cache_.end()) {
        misses_++;
        return false;
    }
    
    auto now = std::chrono::steady_clock::now();
    auto age = now - it->second.timestamp;
    
    if (age > DEFAULT_TTL) {
        LOGD("Cache expired: %s", path.c_str());
        cache_.erase(it);
        misses_++;
        return false;
    }
    
    out = it->second.metadata;
    hits_++;
    return true;
}

void MetadataCache::put(const std::string& path, const Mp3Metadata& metadata) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (cache_.size() >= MAX_ENTRIES && cache_.find(path) == cache_.end()) {
        auto oldest = cache_.begin();
        auto oldestTime = oldest->second.timestamp;
        
        for (auto it = cache_.begin(); it != cache_.end(); ++it) {
            if (it->second.timestamp < oldestTime) {
                oldest = it;
                oldestTime = it->second.timestamp;
            }
        }
        
        LOGD("Cache evicting: %s", oldest->first.c_str());
        cache_.erase(oldest);
    }
    
    CacheEntry entry;
    entry.metadata = metadata;
    entry.metadata.cachedAt = std::chrono::steady_clock::now();
    entry.timestamp = entry.metadata.cachedAt;
    
    cache_[path] = std::move(entry);
}

void MetadataCache::remove(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    cache_.erase(path);
}

size_t MetadataCache::cleanup() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto now = std::chrono::steady_clock::now();
    size_t removed = 0;
    
    for (auto it = cache_.begin(); it != cache_.end();) {
        auto age = now - it->second.timestamp;
        if (age > DEFAULT_TTL) {
            it = cache_.erase(it);
            removed++;
        } else {
            ++it;
        }
    }
    
    return removed;
}

void MetadataCache::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    cache_.clear();
    hits_ = 0;
    misses_ = 0;
}

size_t MetadataCache::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return cache_.size();
}

} // namespace eetgw
