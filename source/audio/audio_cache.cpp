#include "audio_cache.h"
#include <unordered_map>
#include <mutex>

static std::mutex g_cacheMutex;
static std::unordered_map<int, std::vector<float>> g_cache;

void StoreCachedAudio(int key, const std::vector<float>& audio) {
    std::lock_guard<std::mutex> lock(g_cacheMutex);
    g_cache[key] = audio;
}

// Returns audio and removes it — consume once.
std::vector<float> TakeCachedAudio(int key) {
    std::lock_guard<std::mutex> lock(g_cacheMutex);
    auto it = g_cache.find(key);
    if (it == g_cache.end()) return {};
    std::vector<float> out = std::move(it->second);
    g_cache.erase(it);
    return out;
}
