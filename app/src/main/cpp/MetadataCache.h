#pragma once

#include "DataTypes.h"
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <memory>

// =============================================================================
// EETGW — Essa e pra tocar no Galaxy Watch
// MetadataCache.h — Cache LRU com TTL para metadados / LRU cache with TTL for metadata
// =============================================================================

namespace eetgw {

class MetadataCache {
public:
    static constexpr auto DEFAULT_TTL = std::chrono::hours(1);
    static constexpr size_t MAX_ENTRIES = 100;

    static MetadataCache& instance();

    bool get(const std::string& path, Mp3Metadata& out);
    void put(const std::string& path, const Mp3Metadata& metadata);
    void remove(const std::string& path);
    size_t cleanup();
    void clear();
    size_t size() const;
    size_t hitCount() const { return hits_; }
    size_t missCount() const { return misses_; }

private:
    MetadataCache() = default;
    ~MetadataCache() = default;
    MetadataCache(const MetadataCache&) = delete;
    MetadataCache& operator=(const MetadataCache&) = delete;

    struct CacheEntry {
        Mp3Metadata metadata;
        std::chrono::steady_clock::time_point timestamp;
    };

    std::unordered_map<std::string, CacheEntry> cache_;
    mutable std::mutex mutex_;
    size_t hits_ = 0;
    size_t misses_ = 0;
};

} // namespace eetgw
