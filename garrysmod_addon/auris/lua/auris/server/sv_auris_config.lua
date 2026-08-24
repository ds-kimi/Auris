-- Loads lua/auris/config.lua and merges with ConVar overrides.
-- Must run before sv_auris_boot.lua so Boot() has a valid Auris._config.

local DEFAULTS = {
    -- core
    model            = "data/auris/ggml-tiny.en.bin",
    port             = 4000,
    threads          = 4,
    language         = "en",
    debug            = false,
    print_progress   = false,
    print_timestamps = false,
    single_segment   = true,
    no_context       = true,

    -- sampling
    use_beam_search  = false,
    greedy_best_of   = 5,
    beam_size        = 5,

    -- output
    translate        = false,
    detect_language  = false,
    no_timestamps    = false,
    print_special    = false,
    print_realtime   = false,
    debug_mode       = false,
    tdrz_enable      = false,

    -- token timestamps
    token_timestamps     = false,
    thold_pt             = 0.01,
    thold_ptsum          = 0.01,
    max_len              = 0,
    split_on_word        = false,
    max_tokens           = 0,

    -- filtering
    suppress_blank       = true,
    suppress_nst         = false,
    no_speech_thold      = 0.6,
    suppress_regex       = "",
    initial_prompt       = "",
    carry_initial_prompt = false,
    vad_thold            = 0.6,
    freq_thold           = 100.0,

    -- decoding
    temperature      = 0.0,
    temperature_inc  = 0.2,
    entropy_thold    = 2.4,
    logprob_thold    = -1.0,
    max_initial_ts   = 1.0,
    length_penalty   = -1.0,

    -- context
    n_max_text_ctx = 16384,
    offset_ms      = 0,
    duration_ms    = 0,
    audio_ctx      = 0,

    -- capture
    preserve_timeline = false,

    -- remote backend
    openai_api_key = "",
    openai_model   = "whisper-1",
}

---@return table
local function loadFileConfig()
    local ok, result = pcall(include, "auris/config.lua")
    -- If the file is missing or malformed, fall back to defaults silently.
    -- Boot will still succeed; the operator sees normal behaviour.
    if not ok or type(result) ~= "table" then return {} end
    return result
end

---@param cfg table
local function registerConVars(cfg)
    CreateConVar("auris_threads",  tostring(cfg.threads),  FCVAR_ARCHIVE, "Whisper CPU thread count")
    CreateConVar("auris_language", tostring(cfg.language), FCVAR_ARCHIVE, "Transcription language code")
    CreateConVar("auris_debug",    cfg.debug and "1" or "0", FCVAR_ARCHIVE, "Enable Auris debug output")
end

-- ConVar wins over file value so server.cfg takes precedence.
---@param cfg table
local function applyConVarOverrides(cfg)
    cfg.threads  = tonumber(GetConVar("auris_threads"):GetInt())  or cfg.threads
    cfg.language = GetConVar("auris_language"):GetString()        or cfg.language
    cfg.debug    = GetConVar("auris_debug"):GetBool()
end

function Auris.LoadConfig()
    local file = loadFileConfig()
    local cfg  = table.Merge(table.Copy(DEFAULTS), file)
    registerConVars(cfg)
    applyConVarOverrides(cfg)
    Auris._config = cfg
end
