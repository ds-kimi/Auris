// Half the cores leaves the other half to the game tick and the OS; past four
// workers the shared weights stop fitting in cache and throughput flattens.
#include "pool_limits.h"
#include <algorithm>
#include <cctype>
#include <thread>

int ResolveWorkerCount(const WhisperConfig& cfg) {
    if (cfg.workers > 0) return std::min(8, cfg.workers);
    int cores = (int)std::thread::hardware_concurrency();
    return std::min(4, std::max(1, cores / 2));
}

size_t MaxQueueSamples(const WhisperConfig& cfg) {
    float seconds = std::min(600.0f, std::max(1.0f, cfg.max_queue_seconds));
    return (size_t)(seconds * 16000.0f);
}

bool IsAcftModel(const std::string& modelPath) {
    std::string lower = modelPath;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return (char)std::tolower(c); });
    return lower.find("acft") != std::string::npos;
}
