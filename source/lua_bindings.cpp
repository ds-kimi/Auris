// Lua-facing functions for auris module
#include "lua_bindings.h"
#include "auris_context.h"
#include "audio_cache.h"
#include "audio_buffer.h"
#include "voice_hook.h"
#include "steam_voice.h"
#include "auris_config.h"
#include "debug_log.h"
#include "worker_placement.h"

using namespace GarrysMod::Lua;

// auris.Init(modelPath [, skipWhisper]) -> bool
// skipWhisper=true powers the remote (OpenAI) backend: audio capture still
// needs the Steam voice decoder + detour, but whisper.cpp is never loaded
// and the worker thread is never spawned.
LUA_FUNCTION(Whisper_Init) {
    const char* path = LUA->CheckString(1);
    bool skipWhisper = false;
    if (LUA->Top() >= 2 && LUA->IsType(2, Type::Bool)) {
        skipWhisper = LUA->GetBool(2);
    }
    if (!InitSteamVoiceDecoder()) {
        LUA->PushBool(false);
        return 1;
    }
    if (!InstallVoiceHook()) {
        LUA->PushBool(false);
        return 1;
    }
    if (skipWhisper) {
        LUA->PushBool(true);
        return 1;
    }
    LUA->PushBool(InitWhisper(path));
    return 1;
}

// auris.Flush(userid) -> nil
// Flushes buffered audio and runs transcription
LUA_FUNCTION(Whisper_Flush) {
    int userid = (int)LUA->CheckNumber(1);
    auto audio = GetAudioBuffer().Flush(userid);
    if (audio.empty()) return 0;
    QueueTranscription(userid, std::move(audio));
    return 0;
}

// auris.FlushAll() -> nil
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

// auris.FlushRaw(userid) -> string|nil
// Remote backend entry point: returns the captured float32 PCM as a binary
// Lua string without queueing a whisper job. Caller is expected to transport
// the audio elsewhere (e.g. OpenAI /v1/audio/transcriptions) and fire the
// Auris_Transcription hook itself.
LUA_FUNCTION(Whisper_FlushRaw) {
    int userid = (int)LUA->CheckNumber(1);
    auto audio = GetAudioBuffer().Flush(userid);
    if (audio.empty()) {
        LUA->PushNil();
        return 1;
    }
    LUA->PushString(
        reinterpret_cast<const char*>(audio.data()),
        audio.size() * sizeof(float)
    );
    return 1;
}

// auris.Poll() -> sid64, text, audio_binary or nil, key
// key is the voice buffer key: negative entity index for clients without a
// SteamID (sv_lan), where sid64 is "0" for every one of them
LUA_FUNCTION(Whisper_Poll) {
    // the poll timer runs on the main thread: the one place to see its core
    RefreshPlacement();
    TranscriptResult result;
    if (!PollResult(result)) return 0;

    auto sid = GetSteamID64ForKey(result.key);
    WDEBUG("[Auris] Poll key=%d sid=%s\n",
        result.key, sid.c_str());
    LUA->PushString(sid.c_str());
    LUA->PushString(result.text.c_str());

    auto audio = TakeCachedAudio(result.key);
    if (audio.empty()) {
        LUA->PushNil();
    } else {
        LUA->PushString(reinterpret_cast<const char*>(audio.data()),
            audio.size() * sizeof(float));
    }
    LUA->PushNumber((double)result.key);
    return 4;
}

// auris.Shutdown() -> nil
LUA_FUNCTION(Whisper_Shutdown) {
    UninstallVoiceHook();
    ShutdownSteamVoiceDecoder();
    ShutdownWhisper();
    return 0;
}

// auris.Debug(bool) -> nil
LUA_FUNCTION(Whisper_Debug) {
    LUA->CheckType(1, GarrysMod::Lua::Type::Bool);
    g_debugEnabled = LUA->GetBool(1);
    return 0;
}

// auris.IsDebug() -> bool
LUA_FUNCTION(Whisper_IsDebug) {
    LUA->PushBool(g_debugEnabled.load());
    return 1;
}
