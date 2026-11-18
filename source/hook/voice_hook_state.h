#pragma once
#include <mutex>
#include <unordered_map>
#include <cstdint>
#include <detouring/hook.hpp>

extern std::mutex g_steamidMutex;
extern std::unordered_map<int, uint64_t> g_keyToSteamid;
extern Detouring::Hook detour_BroadcastVoiceData;
