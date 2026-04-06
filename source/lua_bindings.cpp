// Lua-facing functions for whisper module
#include "lua_bindings.h"
#include "whisper_context.h"
#include "audio_buffer.h"
#include "udp_listener.h"
#include "steam_voice.h"
#include "whisper_config.h"
#include "debug_log.h"

using namespace GarrysMod::Lua;

// whisper.Init(modelPath) -> bool
LUA_FUNCTION(Whisper_Init) {
    const char* path = LUA->CheckString(1);
    if (!InitSteamVoiceDecoder()) {
        LUA->PushBool(false);
        return 1;
    }
    LUA->PushBool(InitWhisper(path));
    return 1;
}

// whisper.Listen(port) -> nil
LUA_FUNCTION(Whisper_Listen) {
    int port = (int)LUA->CheckNumber(1);
    StartUDPListener(port);
    return 0;
}

// whisper.Flush(userid) -> nil
// Flushes buffered audio and runs transcription
LUA_FUNCTION(Whisper_Flush) {
    int userid = (int)LUA->CheckNumber(1);
    auto audio = GetAudioBuffer().Flush(userid);
    if (audio.empty()) return 0;
    QueueTranscription(userid, std::move(audio));
    return 0;
}

// whisper.FlushAll() -> nil
// Queues all active buffers for background transcription
LUA_FUNCTION(Whisper_FlushAll) {
    auto keys = GetAudioBuffer().GetActiveKeys();
    for (int key : keys) {
        auto audio = GetAudioBuffer().Flush(key);
        if (audio.empty()) continue;
        QueueTranscription(key, std::move(audio));
    }
    return 0;
}

// whisper.Poll() -> userid, text or nil
LUA_FUNCTION(Whisper_Poll) {
    TranscriptResult result;
    if (!PollResult(result)) return 0;

    auto sid = GetSteamID64ForKey(result.key);
    WDEBUG("[Whisper] Poll key=%d sid=%s\n",
        result.key, sid.c_str());
    LUA->PushString(sid.c_str());
    LUA->PushString(result.text.c_str());
    return 2;
}

// whisper.Shutdown() -> nil
LUA_FUNCTION(Whisper_Shutdown) {
    StopUDPListener();
    ShutdownSteamVoiceDecoder();
    ShutdownWhisper();
    return 0;
}

// whisper.Debug(bool) -> nil
LUA_FUNCTION(Whisper_Debug) {
    LUA->CheckType(1, GarrysMod::Lua::Type::Bool);
    g_debugEnabled = LUA->GetBool(1);
    return 0;
}

// whisper.IsDebug() -> bool
LUA_FUNCTION(Whisper_IsDebug) {
    LUA->PushBool(g_debugEnabled.load());
    return 1;
}
