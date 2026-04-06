// GMod module entry point for gm_whisper
#include "GarrysMod/Lua/Interface.h"
#include "lua_bindings.h"
#include "whisper_context.h"
#include "udp_listener.h"
#include "steam_voice.h"

using namespace GarrysMod::Lua;

static void RegisterFunction(
    ILuaBase* LUA, const char* name, CFunc func
) {
    LUA->PushString(name);
    LUA->PushCFunction(func);
    LUA->SetTable(-3);
}

GMOD_MODULE_OPEN() {
    LUA->PushSpecial(SPECIAL_GLOB);
    LUA->CreateTable();

    RegisterFunction(LUA, "Init", Whisper_Init);
    RegisterFunction(LUA, "Listen", Whisper_Listen);
    RegisterFunction(LUA, "Flush", Whisper_Flush);
    RegisterFunction(LUA, "FlushAll", Whisper_FlushAll);
    RegisterFunction(LUA, "Poll", Whisper_Poll);
    RegisterFunction(LUA, "Shutdown", Whisper_Shutdown);
    RegisterFunction(LUA, "Debug", Whisper_Debug);
    RegisterFunction(LUA, "IsDebug", Whisper_IsDebug);
    RegisterFunction(LUA, "SetConfig", Whisper_SetConfig);
    RegisterFunction(LUA, "GetConfig", Whisper_GetConfig);

    LUA->SetField(-2, "whisper");
    LUA->Pop();

    return 0;
}

GMOD_MODULE_CLOSE() {
    StopUDPListener();
    ShutdownSteamVoiceDecoder();
    ShutdownWhisper();
    return 0;
}
