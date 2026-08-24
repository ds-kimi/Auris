-- Operator config. Edit values here; ConVars in server.cfg override these.
-- Restart the map after changes for them to take effect.

return {

    -- =========================================================================
    -- CORE
    -- =========================================================================

    -- Path to the Whisper GGML model binary.
    -- Larger models = higher accuracy, higher CPU/GPU cost, longer latency.
    --   tiny  ~75MB  — fast, good enough for short game chat
    --   base  ~140MB — noticeably better, still real-time on most hardware
    --   small ~460MB — good quality, needs a decent CPU or GPU
    --   medium/large  — not recommended for live game servers
    -- .en.bin variants are English-only but faster than the multilingual equivalents.
    model = "garrysmod/data/auris/ggml-tiny.en.bin",

    -- CPU threads for Whisper inference.
    -- Raise to speed up transcription on servers with spare cores.
    -- Going beyond physical core count gives no benefit and wastes context switches.
    threads = 4,

    -- BCP-47 language code ("en", "de", "fr", "ja", …).
    -- Ignored for .en.bin models (English is hard-coded in those weights).
    -- Set to "" and enable detect_language below for automatic detection.
    language = "en",

    -- Prints detailed C-level debug output to the server console.
    -- Only useful when diagnosing module crashes or audio pipeline issues.
    debug = false,

    -- Prints inference progress to stdout (0 %, 10 %, …).
    -- Clutters the server console; leave off in production.
    print_progress = false,

    -- Includes timestamps in the realtime print stream.
    -- Only relevant when print_realtime (below) is also true.
    print_timestamps = false,

    -- Forces the decoder to emit one output segment regardless of audio length.
    -- Kept true because each flush is one short player utterance, not a long recording.
    -- Setting false lets Whisper split long audio into sentences, which helps for
    -- long speech but adds latency and complexity for game-chat use.
    single_segment = true,

    -- Discards the rolling context from the previous chunk.
    -- Kept true to prevent one player's words bleeding into the next player's result.
    -- Set false only if you are transcribing a single continuous audio stream.
    no_context = true,


    -- =========================================================================
    -- SAMPLING STRATEGY
    -- =========================================================================

    -- false = greedy (fast, picks highest-probability token each step)
    -- true  = beam search (slower, explores multiple hypotheses, often more accurate)
    -- For live game chat greedy is almost always the right choice.
    -- Beam search shines on longer, quieter audio (e.g. recorded interviews).
    use_beam_search = false,

    -- Greedy only. Number of independent greedy runs to compare and pick the best.
    -- 1 = pure greedy (fastest). 5 = default. Values above 5 rarely help.
    greedy_best_of = 3,

    -- Beam search only. Width of the search tree.
    -- Higher = better quality but memory and time grow roughly exponentially.
    -- 5 is the standard value; going above 10 is rarely worth the cost.
    beam_size = 5,


    -- =========================================================================
    -- OUTPUT
    -- =========================================================================

    -- Translate audio to English instead of transcribing in the original language.
    -- Uses Whisper's built-in translation — no extra model required.
    -- Only works with multilingual models (not .en.bin variants).
    translate = false,

    -- Force automatic language detection from the audio even if language is set.
    -- Detection runs once on the first ~30 s of audio then is cached for that chunk.
    -- Costs a small amount of extra compute per transcription.
    detect_language = false,

    -- Strip timestamp tokens from the output entirely.
    -- Distilled models force this to true regardless of this setting.
    -- Enabling it saves a tiny amount of decode work; timestamps are rarely useful
    -- in game-chat addons unless you are building a replay or subtitle system.
    no_timestamps = true,

    -- Include special control tokens (<SOT>, <EOT>, <BEG>, …) in the output text.
    -- Only useful when debugging the decoder; produces noise in production.
    print_special = false,

    -- Stream tokens to stdout as they are decoded (before the segment is finished).
    -- Useful for watching Whisper work in a terminal; noisy in production.
    print_realtime = false,

    -- Extra internal debug logging from the C layer (mel dump, tensor shapes, …).
    -- Separate from the top-level `debug` flag; much more verbose.
    debug_mode = false,

    -- Enable tinydiarize speaker-turn detection.
    -- Requires a model trained with tinydiarize support (not standard ggml models).
    -- When true the model can emit a special SOLM token at speaker boundaries.
    -- Does not identify WHO is speaking, only THAT a turn occurred.
    tdrz_enable = false,


    -- =========================================================================
    -- TOKEN-LEVEL TIMESTAMPS (experimental)
    -- =========================================================================

    -- Compute per-token start/end timestamps using DTW alignment.
    -- Required if you want word-level timing data in the Auris_Transcription callback.
    -- Adds post-processing overhead after each segment is decoded.
    token_timestamps = false,

    -- Minimum probability for a timestamp token to be accepted during DTW alignment.
    -- Raise to require higher confidence on each timestamp boundary.
    -- Lower to accept more uncertain timestamps (can get noisier results).
    thold_pt = 0.01,

    -- Minimum cumulative probability across adjacent timestamp tokens.
    -- Acts as a secondary confidence gate alongside thold_pt.
    thold_ptsum = 0.01,

    -- Split long segments at this many characters (0 = no limit).
    -- Useful for subtitle generation where lines must fit a fixed width.
    -- Has no effect when single_segment = true and the segment is short.
    max_len = 0,

    -- When max_len > 0, split only at word boundaries instead of mid-character.
    -- Prevents truncated words at the end of subtitle lines.
    split_on_word = false,

    -- Hard limit on tokens per segment (0 = no limit).
    -- Forces a segment break after this many tokens regardless of content.
    -- Low values will cut off mid-word; use max_len + split_on_word instead.
    -- 0 = no limit. Non-zero forces a segment break mid-utterance which silently
    -- drops any tokens past the cut when single_segment = true.
    max_tokens = 96,


    -- =========================================================================
    -- FILTERING
    -- =========================================================================

    -- Suppress a leading blank/space token at the start of each segment.
    -- Whisper sometimes emits a leading space before the first word; this removes it.
    -- Almost always desirable; only turn off if you need bit-exact token output.
    suppress_blank = true,

    -- Suppress non-speech tokens: punctuation marks, musical notation symbols, etc.
    -- Useful if downstream code cannot handle these characters.
    -- May reduce transcription quality on naturally punctuated speech.
    suppress_nst = false,

    -- Probability threshold above which a segment is classified as silence / no-speech.
    -- Range: 0.0–1.0. Default 0.6 works well for push-to-talk environments.
    -- Raise (e.g. 0.8) to be more aggressive about discarding silent audio.
    -- Lower (e.g. 0.3) if you are missing faint speech (e.g. quiet microphones).
    no_speech_thold = 0.6,

    -- Lua pattern (POSIX regex) matched against vocabulary tokens.
    -- Tokens matching this pattern have their logit set to -inf before sampling,
    -- effectively preventing them from appearing in output.
    -- Example: "^[%d]+$" would suppress pure-number tokens.
    -- Empty string disables this feature.
    suppress_regex = "",

    -- Text to prepend to the decoder's prompt before transcription begins.
    -- Guides style, vocabulary, and context for the model.
    -- Applies to BOTH backends: whisper.cpp (initial_prompt) and OpenAI (prompt form field).
    -- Example: "Garry's Mod DarkRP voice chat. Vocabulary: prop, physgun, ULX, NLR, mug, raid, PD, mayor."
    -- Capped internally at roughly half the model's text context (~224 tokens for tiny).
    initial_prompt = "Garry's Mod voice chat. C++ module. Opus, whisper.cpp, transcription, open source. DarkRP, prop, physgun, ULX, NLR, mug, raid, PD, mayor, hitman, warrant, base, party, job, salary, printer, Bitcoin, shipment.",

    -- When processing multiple chunks, carry initial_prompt into every chunk.
    -- false = only the most recent transcribed text is used as rolling context.
    -- true  = initial_prompt text is always prepended, keeping style consistent.
    carry_initial_prompt = false,

    -- Voice Activity Detection energy threshold. Gates chunks before Whisper
    -- sees them, saving compute and preventing hallucinated text on silence
    -- (e.g. "Thanks for watching", "[Music]").
    -- Compares energy of the last 1000 ms against the whole-chunk average; a
    -- chunk whose tail is quieter than (vad_thold * average) is treated as a
    -- completed utterance and transcribed, otherwise it is skipped.
    -- 0.6 matches upstream whisper.cpp stream/command example defaults.
    -- Set to 0 (or negative) to disable VAD entirely and transcribe every chunk.
    vad_thold = 0,

    -- High-pass filter cutoff in Hz applied before VAD energy calculation.
    -- Strips server-room fans, desk rumble and mic DC offset that would
    -- otherwise register as "energy" and defeat VAD. 100 Hz is the upstream
    -- default and leaves the full speech band intact.
    -- Set to 0 to disable the filter (VAD then runs on raw PCM).
    freq_thold = 0,


    -- =========================================================================
    -- DECODING
    -- =========================================================================

    -- Initial sampling temperature (0.0 = deterministic greedy).
    -- Higher values inject randomness into token selection.
    -- Whisper starts at this value and increments by temperature_inc on fallbacks.
    -- Leave at 0.0 for fastest, most consistent game-chat transcription.
    temperature = 0.0,

    -- How much to increase temperature on each fallback retry.
    -- Fallback triggers when quality checks (entropy, logprob) fail at the current temp.
    -- 0.2 means retries at 0.2, 0.4, 0.6, 0.8, 1.0 before giving up.
    -- Lower = finer retry steps (more attempts); higher = coarser (fewer).
    temperature_inc = 0.2,

    -- Minimum output entropy before the result is considered repetitive/degenerate.
    -- If entropy drops below this (for outputs > 32 tokens), a higher temperature is tried.
    -- Raise to catch more repetitive outputs; lower to allow more uniform outputs through.
    entropy_thold = 2.4,

    -- Average log-probability threshold. Results below this trigger a fallback retry.
    -- -1.0 disables this check. Raise toward 0.0 to require higher confidence output.
    -- Only triggers a retry when no_speech_thold is also not exceeded.
    logprob_thold = -1.0,

    -- Maximum timestamp allowed for the very first emitted token (seconds).
    -- Prevents the model from placing the start of speech unreasonably late.
    -- 1.0 s means the model cannot claim speech begins after 1 second of silence.
    -- Raise if legitimate speech regularly starts more than 1 s into the clip.
    max_initial_ts = 1.0,

    -- Length penalty applied to beam search candidate scores.
    -- Positive values favour longer sequences; negative values penalise them.
    -- -1.0 disables the penalty (recommended for short game-chat utterances).
    -- Only affects beam search; ignored in greedy mode.
    length_penalty = -1.0,


    -- =========================================================================
    -- CONTEXT
    -- =========================================================================

    -- Maximum number of text context tokens the decoder can attend to.
    -- 16384 is the architectural maximum for most models; lowering saves VRAM/RAM
    -- but may reduce coherence on longer audio.
    n_max_text_ctx = 16384,

    -- Millisecond offset into the audio to start transcription from.
    -- 0 = start from the beginning of the clip.
    -- Useful only if you are slicing a longer recording before feeding it to Whisper.
    offset_ms = 0,

    -- How many milliseconds of audio to process (0 = entire clip).
    -- Combine with offset_ms to create a sliding window over long recordings.
    -- Not useful for game-chat where each flush is already one short utterance.
    duration_ms = 0,

    -- Override the model's audio context window size (0 = use model default).
    -- Cannot exceed the model's built-in maximum.
    -- Smaller values reduce encoder compute at the cost of losing distant audio.
    audio_ctx = 0,


    -- =========================================================================
    -- CAPTURE
    -- =========================================================================

    -- Keep silence in the captured audio so a clip is the same length as the
    -- time it took to speak. Off by default: transcription does not care where
    -- the pauses were, and dropping them keeps buffers and inference smaller.
    -- Turn it on only when something consumes the audio itself rather than the
    -- text — a replay system, evidence capture, anything that plays a clip back
    -- in step with what was happening while it was recorded. With it off, a
    -- sentence with pauses in it comes out shorter than it was spoken and every
    -- word after a pause sits earlier than it happened.
    -- Addons can also flip this at runtime with auris.SetPreserveTimeline(bool).
    preserve_timeline = false,


    -- =========================================================================
    -- REMOTE BACKEND (OpenAI)
    -- =========================================================================

    -- OpenAI API key. When non-empty, Auris skips loading whisper.cpp entirely
    -- and forwards every captured voice clip to the OpenAI speech-to-text API
    -- (https://api.openai.com/v1/audio/transcriptions).
    -- Leave "" (default) to keep the local whisper.cpp backend — no network I/O,
    -- full privacy, zero per-clip cost, but whatever CPU/GPU load the local
    -- model requires.
    -- Keys belong here
    openai_api_key = "",

    -- OpenAI transcription model. Ignored while openai_api_key is "".
    -- Common options (see https://developers.openai.com/api/docs/guides/speech-to-text):
    --   "whisper-1"               — legacy, cheapest, broadest language support
    --   "gpt-4o-mini-transcribe"  — recommended default for English game chat
    --   "gpt-4o-transcribe"       — highest accuracy, highest cost and latency
    openai_model = "whisper-1",
}
