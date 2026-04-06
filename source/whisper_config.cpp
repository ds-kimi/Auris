// Thread-safe whisper config storage
#include "whisper_config.h"

static std::mutex g_cfgMutex;
static WhisperConfig g_config;

WhisperConfig GetWhisperConfig() {
    std::lock_guard<std::mutex> lock(g_cfgMutex);
    return g_config;
}

void SetWhisperConfig(const WhisperConfig& cfg) {
    std::lock_guard<std::mutex> lock(g_cfgMutex);
    g_config = cfg;
}
