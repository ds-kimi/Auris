-- Initialises the auris C++ module.
-- Sets Auris._ready = true only on full success so the poll loop
-- and submodules have a reliable gate to check before acting.

-- .en.bin models hard-code English; passing a different language to them
-- causes auris to silently produce garbage output.
---@param cfg table
---@return string
local function resolveLanguage(cfg)
    if string.find(cfg.model, "%.en%.bin$") then return "en" end
    return cfg.language
end

---@param cfg table
---@return table
local function buildWhisperConfig(cfg)
    return {
        -- core
        language         = resolveLanguage(cfg),
        n_threads        = cfg.threads,
        print_progress   = cfg.print_progress,
        print_timestamps = cfg.print_timestamps,
        single_segment   = cfg.single_segment,
        no_context       = cfg.no_context,

        -- sampling
        use_beam_search  = cfg.use_beam_search,
        greedy_best_of   = cfg.greedy_best_of,
        beam_size        = cfg.beam_size,

        -- output
        translate             = cfg.translate,
        detect_language       = cfg.detect_language,
        no_timestamps         = cfg.no_timestamps,
        print_special         = cfg.print_special,
        print_realtime        = cfg.print_realtime,
        debug_mode            = cfg.debug_mode,
        tdrz_enable           = cfg.tdrz_enable,

        -- token timestamps
        token_timestamps     = cfg.token_timestamps,
        thold_pt             = cfg.thold_pt,
        thold_ptsum          = cfg.thold_ptsum,
        max_len              = cfg.max_len,
        split_on_word        = cfg.split_on_word,
        max_tokens           = cfg.max_tokens,

        -- filtering
        suppress_blank       = cfg.suppress_blank,
        suppress_nst         = cfg.suppress_nst,
        no_speech_thold      = cfg.no_speech_thold,
        suppress_regex       = cfg.suppress_regex,
        initial_prompt       = cfg.initial_prompt,
        carry_initial_prompt = cfg.carry_initial_prompt,
        vad_thold            = cfg.vad_thold,
        freq_thold           = cfg.freq_thold,

        -- decoding
        temperature      = cfg.temperature,
        temperature_inc  = cfg.temperature_inc,
        entropy_thold    = cfg.entropy_thold,
        logprob_thold    = cfg.logprob_thold,
        max_initial_ts   = cfg.max_initial_ts,
        length_penalty   = cfg.length_penalty,

        -- context
        n_max_text_ctx = cfg.n_max_text_ctx,
        offset_ms      = cfg.offset_ms,
        duration_ms    = cfg.duration_ms,
        audio_ctx      = cfg.audio_ctx,
    }
end

function Auris.Boot()
    local cfg = Auris._config
    -- Remote backend still needs the Steam voice detour to decode Opus,
    -- but whisper.cpp and its worker thread are skipped entirely.
    local useRemote = cfg.openai_api_key ~= ""

    -- _G.__AurisInitDone survives lua_refresh (unlike the Auris table which is
    -- re-declared each run). Skip Init() when the C++ module is already running.
    if not _G.__AurisInitDone then
        if not auris.Init(cfg.model, useRemote) then
            ErrorNoHalt("[Auris] Failed to init (model: " .. tostring(cfg.model) .. ")\n")
            return
        end
        _G.__AurisInitDone = true
    end

    if not useRemote then
        auris.SetConfig(buildWhisperConfig(cfg))
    end

    -- Capture fidelity is not a whisper setting: it changes what lands in the
    -- buffer, so it applies to both backends and to anything reading the audio
    -- with FlushRaw. Guarded so an older module binary still boots.
    if isfunction(auris.SetPreserveTimeline) then
        auris.SetPreserveTimeline(cfg.preserve_timeline == true)
    end

    auris.Debug(cfg.debug)

    Auris._ready = true
    print("[Auris] v" .. (auris.VERSION or "?") .. " ready")
end
