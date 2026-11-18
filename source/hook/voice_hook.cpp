#include "voice_hook.h"
#include "voice_hook_state.h"
#include "audio_buffer.h"
#include "steam_voice.h"
#include "debug_log.h"
#include "voice_key.h"

#include <GarrysMod/FactoryLoader.hpp>
#include <GarrysMod/Symbol.hpp>
#include <scanning/symbolfinder.hpp>
#include <detouring/hook.hpp>
#include <iclient.h>

#include <cstdint>
#include <cstring>
#include <vector>

std::mutex g_steamidMutex;
std::unordered_map<int, uint64_t> g_keyToSteamid;
Detouring::Hook detour_BroadcastVoiceData;

typedef void (*SV_BroadcastVoiceData)(IClient* cl, int nBytes, char* data, int64 xuid);
// Thanks to gm_8bit for the signatures and general reverse engineering of the voice data flow!
#ifdef SYSTEM_WINDOWS
    static const std::vector<Symbol> BroadcastVoiceSyms = {
#if defined ARCHITECTURE_X86
        Symbol::FromSignature("\x55\x8B\xEC\xA1****\x83\xEC\x50\x83\x78\x30\x00\x0F\x84****\x53\x8D\x4D\xD8\xC6\x45\xB4\x01\xC7\x45*****"),
        Symbol::FromSignature("\x55\x8B\xEC\x8B\x0D****\x83\xEC\x58\x81\xF9****"),
        Symbol::FromSignature("\x55\x8B\xEC\xA1****\x83\xEC\x50"),
#elif defined ARCHITECTURE_X86_64
        Symbol::FromSignature("\x48\x89\x5C\x24*\x56\x57\x41\x56\x48\x81\xEC****\x8B\xF2\x4C\x8B\xF1"),
#endif
    };
#endif

#ifdef SYSTEM_LINUX
    static const std::vector<Symbol> BroadcastVoiceSyms = {
        Symbol::FromName("_Z21SV_BroadcastVoiceDataP7IClientiPcx"),
#if defined ARCHITECTURE_X86
        Symbol::FromSignature("\x55\x89\xe5\x57\x56\x53\x83\xec\x6c\xa1****\x8b\x75\x14\x8b\x7d\x18\x8b\x50\x48\x85\xd2"),
#else
        Symbol::FromSignature("\x55\x48\x8D\x05****\x48\x89\xE5\x41\x57\x41\x56\x41\x89\xF6\x41\x55\x49\x89\xFD\x41\x54\x49\x89\xD4\x53\x48\x89\xCB\x48\x81\xEC****\x48\x8B\x3D****\x48\x39\xC7\x74\x25"),
#endif
    };
#endif


static uint64_t ExtractServerSteamID(IClient* cl) {
#if defined ARCHITECTURE_X86
    return *(uint64_t*)((char*)cl + 181);
#else
    return *(uint64_t*)((char*)cl + 189);
#endif
}

static int g_packetCount = 0;

static void IngestVoicePacket(int key, uint64_t sid64, const char* data, int nBytes) {
    if (key == 0 || nBytes <= (int)sizeof(uint64_t)) return;

    char buf[20 * 1024];
    if (nBytes > (int)sizeof(buf)) return;

    *(uint64_t*)buf = sid64;
    std::memcpy(buf + sizeof(uint64_t),
                data + sizeof(uint64_t),
                nBytes - sizeof(uint64_t));

    {
        std::lock_guard<std::mutex> lock(g_steamidMutex);
        g_keyToSteamid[key] = sid64;
    }

    auto pcm = DecodeSteamVoice(key, (const uint8_t*)buf, nBytes);

    g_packetCount++;
    if (g_packetCount <= 5) {
        WDEBUG("[Auris] Packet #%d len=%d key=%d steamid=%llu decoded=%d\n",
            g_packetCount, nBytes, key,
            (unsigned long long)sid64, (int)pcm.size());
    }

    if (!pcm.empty()) {
        GetAudioBuffer().Append(key, pcm.data(), (int)pcm.size());
    }
}

static void hook_BroadcastVoiceData(IClient* cl, int nBytes, char* data, int64 xuid) {
    if (cl && nBytes > (int)sizeof(uint64_t)) {
        uint64_t sid64 = ExtractServerSteamID(cl);
        IngestVoicePacket(VoiceKeyFor(cl, sid64), sid64, data, nBytes);
    }
    detour_BroadcastVoiceData.GetTrampoline<SV_BroadcastVoiceData>()(cl, nBytes, data, xuid);
}

bool InstallVoiceHook() {
    SourceSDK::ModuleLoader engine_loader("engine");
    SymbolFinder symfinder;
    void* sv_bcast = nullptr;

    for (const auto& sym : BroadcastVoiceSyms) {
        sv_bcast = symfinder.Resolve(engine_loader.GetModule(), sym.name.c_str(), sym.length);
        if (sv_bcast) break;
    }

    if (!sv_bcast) {
        WDEBUG("[Auris] SV_BroadcastVoiceData symbol not found\n");
        return false;
    }

    detour_BroadcastVoiceData.Create(
        Detouring::Hook::Target(sv_bcast),
        reinterpret_cast<void*>(&hook_BroadcastVoiceData));
    detour_BroadcastVoiceData.Enable();
    WDEBUG("[Auris] Voice hook installed\n");
    return true;
}
