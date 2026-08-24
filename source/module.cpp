// GMod module entry point for gm_auris
#include "GarrysMod/Lua/Interface.h"
#include "lua_bindings.h"
#include "auris_context.h"
#include "voice_hook.h"
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
    RegisterFunction(LUA, "Flush", Whisper_Flush);
    RegisterFunction(LUA, "FlushAll", Whisper_FlushAll);
    RegisterFunction(LUA, "FlushRaw", Whisper_FlushRaw);
    RegisterFunction(LUA, "Poll", Whisper_Poll);
    RegisterFunction(LUA, "Shutdown", Whisper_Shutdown);
    RegisterFunction(LUA, "Debug", Whisper_Debug);
    RegisterFunction(LUA, "IsDebug", Whisper_IsDebug);
    RegisterFunction(LUA, "SetConfig", Whisper_SetConfig);
    RegisterFunction(LUA, "GetConfig", Whisper_GetConfig);
    RegisterFunction(LUA, "SetPreserveTimeline", Whisper_SetPreserveTimeline);
    RegisterFunction(LUA, "PreservesTimeline", Whisper_PreservesTimeline);

    LUA->PushString("VERSION");
    LUA->PushString(AURIS_VERSION);
    LUA->SetTable(-3);

    LUA->SetField(-2, "auris");
    LUA->Pop();

    return 0;
}

GMOD_MODULE_CLOSE() {
    UninstallVoiceHook();
    ShutdownSteamVoiceDecoder();
    ShutdownWhisper();
    return 0;
}
