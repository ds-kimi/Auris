#include "hook/voice_hook.h"
#include "hook/voice_hook_state.h"

#include <cstdio>

void UninstallVoiceHook() {
    if (detour_BroadcastVoiceData.IsValid()) {
        detour_BroadcastVoiceData.Disable();
        detour_BroadcastVoiceData.Destroy();
    }

    std::lock_guard<std::mutex> lock(g_steamidMutex);
    g_keyToSteamid.clear();
}

std::string GetSteamID64ForKey(int key) {
    std::lock_guard<std::mutex> lock(g_steamidMutex);
    auto it = g_keyToSteamid.find(key);
    if (it == g_keyToSteamid.end()) return "";
    char buf[32];
    snprintf(buf, sizeof(buf), "%llu",
        (unsigned long long)it->second);
    return std::string(buf);
}
