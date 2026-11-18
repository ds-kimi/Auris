// Lua bindings for auris config get/set
#include "core/lua_bindings.h"
#include "core/auris_config.h"

using namespace GarrysMod::Lua;

static bool GetBoolField(ILuaBase* LUA, const char* key, bool def) {
    LUA->GetField(-1, key);
    bool val = LUA->IsType(-1, Type::Bool) ? LUA->GetBool(-1) : def;
    LUA->Pop();
    return val;
}

static int GetIntField(ILuaBase* LUA, const char* key, int def) {
    LUA->GetField(-1, key);
    int val = LUA->IsType(-1, Type::Number) ? (int)LUA->GetNumber(-1) : def;
    LUA->Pop();
    return val;
}

static float GetFloatField(ILuaBase* LUA, const char* key, float def) {
    LUA->GetField(-1, key);
    float val = LUA->IsType(-1, Type::Number) ? (float)LUA->GetNumber(-1) : def;
    LUA->Pop();
    return val;
}

static std::string GetStrField(ILuaBase* LUA, const char* key, const char* def) {
    LUA->GetField(-1, key);
    std::string val = LUA->IsType(-1, Type::String) ? LUA->GetString(-1) : def;
    LUA->Pop();
    return val;
}

// auris.SetConfig(table) -> nil
LUA_FUNCTION(Whisper_SetConfig) {
    LUA->CheckType(1, Type::Table);
    LUA->Push(1);

    WhisperConfig cfg = GetWhisperConfig();

    // core
    cfg.print_progress   = GetBoolField(LUA, "print_progress",   cfg.print_progress);
    cfg.print_timestamps = GetBoolField(LUA, "print_timestamps", cfg.print_timestamps);
    cfg.single_segment   = GetBoolField(LUA, "single_segment",   cfg.single_segment);
    cfg.no_context       = GetBoolField(LUA, "no_context",       cfg.no_context);
    cfg.language         = GetStrField( LUA, "language",         cfg.language.c_str());
    cfg.n_threads        = GetIntField( LUA, "n_threads",        cfg.n_threads);
    cfg.workers           = GetIntField(  LUA, "workers",           cfg.workers);
    cfg.max_queue_seconds = GetFloatField(LUA, "max_queue_seconds", cfg.max_queue_seconds);
    cfg.vad_model         = GetStrField(  LUA, "vad_model",         cfg.vad_model.c_str());
    cfg.affinity          = GetBoolField( LUA, "affinity",          cfg.affinity);
    cfg.pin_main_thread   = GetBoolField( LUA, "pin_main_thread",   cfg.pin_main_thread);
    cfg.worker_cores      = GetStrField(  LUA, "worker_cores",      cfg.worker_cores.c_str());
    cfg.auto_workers      = GetBoolField( LUA, "auto_workers",      cfg.auto_workers);
    cfg.backend           = GetStrField(  LUA, "backend",           cfg.backend.c_str());
    cfg.onnx_model_type   = GetStrField(  LUA, "onnx_model_type",   cfg.onnx_model_type.c_str());
    cfg.onnx_model_dir    = GetStrField(  LUA, "onnx_model_dir",    cfg.onnx_model_dir.c_str());

    // sampling
    cfg.use_beam_search  = GetBoolField(LUA, "use_beam_search",  cfg.use_beam_search);
    cfg.greedy_best_of   = GetIntField( LUA, "greedy_best_of",   cfg.greedy_best_of);
    cfg.beam_size        = GetIntField( LUA, "beam_size",        cfg.beam_size);

    // output
    cfg.translate             = GetBoolField(LUA, "translate",             cfg.translate);
    cfg.detect_language       = GetBoolField(LUA, "detect_language",       cfg.detect_language);
    cfg.no_timestamps         = GetBoolField(LUA, "no_timestamps",         cfg.no_timestamps);
    cfg.print_special         = GetBoolField(LUA, "print_special",         cfg.print_special);
    cfg.print_realtime        = GetBoolField(LUA, "print_realtime",        cfg.print_realtime);
    cfg.debug_mode            = GetBoolField(LUA, "debug_mode",            cfg.debug_mode);
    cfg.tdrz_enable           = GetBoolField(LUA, "tdrz_enable",           cfg.tdrz_enable);

    // token timestamps
    cfg.token_timestamps = GetBoolField( LUA, "token_timestamps", cfg.token_timestamps);
    cfg.thold_pt         = GetFloatField(LUA, "thold_pt",         cfg.thold_pt);
    cfg.thold_ptsum      = GetFloatField(LUA, "thold_ptsum",      cfg.thold_ptsum);
    cfg.max_len          = GetIntField(  LUA, "max_len",          cfg.max_len);
    cfg.split_on_word    = GetBoolField( LUA, "split_on_word",    cfg.split_on_word);
    cfg.max_tokens       = GetIntField(  LUA, "max_tokens",       cfg.max_tokens);

    // filtering
    cfg.suppress_blank       = GetBoolField( LUA, "suppress_blank",       cfg.suppress_blank);
    cfg.suppress_nst         = GetBoolField( LUA, "suppress_nst",         cfg.suppress_nst);
    cfg.no_speech_thold      = GetFloatField(LUA, "no_speech_thold",      cfg.no_speech_thold);
    cfg.suppress_regex       = GetStrField(  LUA, "suppress_regex",       cfg.suppress_regex.c_str());
    cfg.initial_prompt       = GetStrField(  LUA, "initial_prompt",       cfg.initial_prompt.c_str());
    cfg.carry_initial_prompt = GetBoolField( LUA, "carry_initial_prompt", cfg.carry_initial_prompt);
    cfg.vad_thold            = GetFloatField(LUA, "vad_thold",            cfg.vad_thold);
    cfg.freq_thold           = GetFloatField(LUA, "freq_thold",           cfg.freq_thold);

    // decoding
    cfg.temperature     = GetFloatField(LUA, "temperature",     cfg.temperature);
    cfg.temperature_inc = GetFloatField(LUA, "temperature_inc", cfg.temperature_inc);
    cfg.entropy_thold   = GetFloatField(LUA, "entropy_thold",   cfg.entropy_thold);
    cfg.logprob_thold   = GetFloatField(LUA, "logprob_thold",   cfg.logprob_thold);
    cfg.max_initial_ts  = GetFloatField(LUA, "max_initial_ts",  cfg.max_initial_ts);
    cfg.length_penalty  = GetFloatField(LUA, "length_penalty",  cfg.length_penalty);

    // context
    cfg.n_max_text_ctx = GetIntField(LUA, "n_max_text_ctx", cfg.n_max_text_ctx);
    cfg.offset_ms      = GetIntField(LUA, "offset_ms",      cfg.offset_ms);
    cfg.duration_ms    = GetIntField(LUA, "duration_ms",    cfg.duration_ms);
    cfg.audio_ctx      = GetIntField(LUA, "audio_ctx",      cfg.audio_ctx);
    cfg.dynamic_audio_ctx = GetBoolField(LUA, "dynamic_audio_ctx", cfg.dynamic_audio_ctx);

    LUA->Pop();
    SetWhisperConfig(cfg);
    return 0;
}

// auris.GetConfig() -> table
LUA_FUNCTION(Whisper_GetConfig) {
    WhisperConfig cfg = GetWhisperConfig();
    LUA->CreateTable();

    // core
    LUA->PushBool(cfg.print_progress);   LUA->SetField(-2, "print_progress");
    LUA->PushBool(cfg.print_timestamps); LUA->SetField(-2, "print_timestamps");
    LUA->PushBool(cfg.single_segment);   LUA->SetField(-2, "single_segment");
    LUA->PushBool(cfg.no_context);       LUA->SetField(-2, "no_context");
    LUA->PushString(cfg.language.c_str()); LUA->SetField(-2, "language");
    LUA->PushNumber(cfg.n_threads);      LUA->SetField(-2, "n_threads");
    LUA->PushNumber(cfg.workers);           LUA->SetField(-2, "workers");
    LUA->PushNumber(cfg.max_queue_seconds); LUA->SetField(-2, "max_queue_seconds");
    LUA->PushString(cfg.vad_model.c_str()); LUA->SetField(-2, "vad_model");
    LUA->PushBool(cfg.affinity);            LUA->SetField(-2, "affinity");
    LUA->PushBool(cfg.pin_main_thread);     LUA->SetField(-2, "pin_main_thread");
    LUA->PushString(cfg.worker_cores.c_str()); LUA->SetField(-2, "worker_cores");
    LUA->PushBool(cfg.auto_workers);        LUA->SetField(-2, "auto_workers");
    LUA->PushString(cfg.backend.c_str());         LUA->SetField(-2, "backend");
    LUA->PushString(cfg.onnx_model_type.c_str()); LUA->SetField(-2, "onnx_model_type");
    LUA->PushString(cfg.onnx_model_dir.c_str());  LUA->SetField(-2, "onnx_model_dir");

    // sampling
    LUA->PushBool(cfg.use_beam_search);  LUA->SetField(-2, "use_beam_search");
    LUA->PushNumber(cfg.greedy_best_of); LUA->SetField(-2, "greedy_best_of");
    LUA->PushNumber(cfg.beam_size);      LUA->SetField(-2, "beam_size");

    // output
    LUA->PushBool(cfg.translate);        LUA->SetField(-2, "translate");
    LUA->PushBool(cfg.detect_language);  LUA->SetField(-2, "detect_language");
    LUA->PushBool(cfg.no_timestamps);    LUA->SetField(-2, "no_timestamps");
    LUA->PushBool(cfg.print_special);    LUA->SetField(-2, "print_special");
    LUA->PushBool(cfg.print_realtime);   LUA->SetField(-2, "print_realtime");
    LUA->PushBool(cfg.debug_mode);       LUA->SetField(-2, "debug_mode");
    LUA->PushBool(cfg.tdrz_enable);      LUA->SetField(-2, "tdrz_enable");

    // token timestamps
    LUA->PushBool(cfg.token_timestamps); LUA->SetField(-2, "token_timestamps");
    LUA->PushNumber(cfg.thold_pt);       LUA->SetField(-2, "thold_pt");
    LUA->PushNumber(cfg.thold_ptsum);    LUA->SetField(-2, "thold_ptsum");
    LUA->PushNumber(cfg.max_len);        LUA->SetField(-2, "max_len");
    LUA->PushBool(cfg.split_on_word);    LUA->SetField(-2, "split_on_word");
    LUA->PushNumber(cfg.max_tokens);     LUA->SetField(-2, "max_tokens");

    // filtering
    LUA->PushBool(cfg.suppress_blank);               LUA->SetField(-2, "suppress_blank");
    LUA->PushBool(cfg.suppress_nst);                 LUA->SetField(-2, "suppress_nst");
    LUA->PushNumber(cfg.no_speech_thold);            LUA->SetField(-2, "no_speech_thold");
    LUA->PushString(cfg.suppress_regex.c_str());     LUA->SetField(-2, "suppress_regex");
    LUA->PushString(cfg.initial_prompt.c_str());     LUA->SetField(-2, "initial_prompt");
    LUA->PushBool(cfg.carry_initial_prompt);         LUA->SetField(-2, "carry_initial_prompt");
    LUA->PushNumber(cfg.vad_thold);                  LUA->SetField(-2, "vad_thold");
    LUA->PushNumber(cfg.freq_thold);                 LUA->SetField(-2, "freq_thold");

    // decoding
    LUA->PushNumber(cfg.temperature);     LUA->SetField(-2, "temperature");
    LUA->PushNumber(cfg.temperature_inc); LUA->SetField(-2, "temperature_inc");
    LUA->PushNumber(cfg.entropy_thold);   LUA->SetField(-2, "entropy_thold");
    LUA->PushNumber(cfg.logprob_thold);   LUA->SetField(-2, "logprob_thold");
    LUA->PushNumber(cfg.max_initial_ts);  LUA->SetField(-2, "max_initial_ts");
    LUA->PushNumber(cfg.length_penalty);  LUA->SetField(-2, "length_penalty");

    // context
    LUA->PushNumber(cfg.n_max_text_ctx); LUA->SetField(-2, "n_max_text_ctx");
    LUA->PushNumber(cfg.offset_ms);      LUA->SetField(-2, "offset_ms");
    LUA->PushNumber(cfg.duration_ms);    LUA->SetField(-2, "duration_ms");
    LUA->PushNumber(cfg.audio_ctx);      LUA->SetField(-2, "audio_ctx");
    LUA->PushBool(cfg.dynamic_audio_ctx); LUA->SetField(-2, "dynamic_audio_ctx");

    return 1;
}
