// Lua bindings for whisper config get/set
#include "lua_bindings.h"
#include "whisper_config.h"

using namespace GarrysMod::Lua;

// Read a bool field from table at stack top
static bool GetBoolField(
    ILuaBase* LUA, const char* key, bool def
) {
    LUA->GetField(-1, key);
    bool val = LUA->IsType(-1, Type::Bool)
        ? LUA->GetBool(-1) : def;
    LUA->Pop();
    return val;
}

// Read an int field from table at stack top
static int GetIntField(
    ILuaBase* LUA, const char* key, int def
) {
    LUA->GetField(-1, key);
    int val = LUA->IsType(-1, Type::Number)
        ? (int)LUA->GetNumber(-1) : def;
    LUA->Pop();
    return val;
}

// Read a string field from table at stack top
static std::string GetStrField(
    ILuaBase* LUA, const char* key, const char* def
) {
    LUA->GetField(-1, key);
    std::string val = LUA->IsType(-1, Type::String)
        ? LUA->GetString(-1) : def;
    LUA->Pop();
    return val;
}

// whisper.SetConfig(table) -> nil
LUA_FUNCTION(Whisper_SetConfig) {
    LUA->CheckType(1, Type::Table);
    LUA->Push(1);

    WhisperConfig cfg = GetWhisperConfig();
    cfg.print_progress = GetBoolField(
        LUA, "print_progress", cfg.print_progress);
    cfg.print_timestamps = GetBoolField(
        LUA, "print_timestamps", cfg.print_timestamps);
    cfg.single_segment = GetBoolField(
        LUA, "single_segment", cfg.single_segment);
    cfg.no_context = GetBoolField(
        LUA, "no_context", cfg.no_context);
    cfg.language = GetStrField(
        LUA, "language", cfg.language.c_str());
    cfg.n_threads = GetIntField(
        LUA, "n_threads", cfg.n_threads);

    LUA->Pop();
    SetWhisperConfig(cfg);
    return 0;
}

// whisper.GetConfig() -> table
LUA_FUNCTION(Whisper_GetConfig) {
    WhisperConfig cfg = GetWhisperConfig();
    LUA->CreateTable();

    LUA->PushBool(cfg.print_progress);
    LUA->SetField(-2, "print_progress");
    LUA->PushBool(cfg.print_timestamps);
    LUA->SetField(-2, "print_timestamps");
    LUA->PushBool(cfg.single_segment);
    LUA->SetField(-2, "single_segment");
    LUA->PushBool(cfg.no_context);
    LUA->SetField(-2, "no_context");
    LUA->PushString(cfg.language.c_str());
    LUA->SetField(-2, "language");
    LUA->PushNumber(cfg.n_threads);
    LUA->SetField(-2, "n_threads");

    return 1;
}
