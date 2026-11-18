// Maps WhisperConfig to whisper_full_params, plus per-clip audio_ctx sizing
#include "whisper_params.h"
#include "debug_log.h"
#include <algorithm>

static void FillCoreParams(const WhisperConfig& cfg, whisper_full_params& w) {
    w.print_progress   = cfg.print_progress;
    w.print_timestamps = cfg.print_timestamps;
    w.single_segment   = cfg.single_segment;
    w.no_context       = cfg.no_context;
    w.language         = cfg.language.empty() ? nullptr : cfg.language.c_str();
    w.n_threads        = cfg.n_threads;

    w.greedy.best_of        = cfg.greedy_best_of;
    w.beam_search.beam_size = cfg.beam_size;

    w.translate       = cfg.translate;
    w.detect_language = cfg.detect_language;
    w.no_timestamps   = cfg.no_timestamps;
    w.print_special   = cfg.print_special;
    w.print_realtime  = cfg.print_realtime;
    w.debug_mode      = cfg.debug_mode;
    w.tdrz_enable     = cfg.tdrz_enable;
}

static void FillFilterParams(const WhisperConfig& cfg, whisper_full_params& w) {
    w.token_timestamps = cfg.token_timestamps;
    w.thold_pt         = cfg.thold_pt;
    w.thold_ptsum      = cfg.thold_ptsum;
    w.max_len          = cfg.max_len;
    w.split_on_word    = cfg.split_on_word;
    w.max_tokens       = cfg.max_tokens;

    w.suppress_blank       = cfg.suppress_blank;
    w.suppress_nst         = cfg.suppress_nst;
    w.no_speech_thold      = cfg.no_speech_thold;
    w.suppress_regex       = cfg.suppress_regex.empty() ? nullptr : cfg.suppress_regex.c_str();
    w.initial_prompt       = cfg.initial_prompt.empty() ? nullptr : cfg.initial_prompt.c_str();
    w.carry_initial_prompt = cfg.carry_initial_prompt;
}

// The encoder runs on a fixed 30 s / 1500-frame window; ACFT models were fine-tuned to stay
// accurate when it is cut to the clip length, which is where most of the CPU saving is.
// Stock models loop or hallucinate at reduced context, so they always keep the full window.
static int ChooseAudioCtx(const WhisperConfig& cfg, int nSamples, bool acftModel) {
    if (cfg.audio_ctx > 0) return cfg.audio_ctx;
    if (!acftModel || !cfg.dynamic_audio_ctx) return 0;
    // 16 kHz / 320 = 50 encoder frames per second; +32 frames of slack for the tail
    if (nSamples > 29 * 16000) return 0;
    int ctx = (nSamples + 319) / 320 + 32;
    return std::min(std::max(ctx, 160), 1500);
}

whisper_full_params BuildWhisperParams(const WhisperConfig& cfg, int nSamples, bool acftModel) {
    whisper_full_params w = whisper_full_default_params(
        cfg.use_beam_search ? WHISPER_SAMPLING_BEAM_SEARCH : WHISPER_SAMPLING_GREEDY
    );
    FillCoreParams(cfg, w);
    FillFilterParams(cfg, w);

    w.temperature     = cfg.temperature;
    w.temperature_inc = cfg.temperature_inc;
    w.entropy_thold   = cfg.entropy_thold;
    w.logprob_thold   = cfg.logprob_thold;
    w.max_initial_ts  = cfg.max_initial_ts;
    w.length_penalty  = cfg.length_penalty;

    w.n_max_text_ctx = cfg.n_max_text_ctx;
    w.offset_ms      = cfg.offset_ms;
    w.duration_ms    = cfg.duration_ms;
    w.audio_ctx      = ChooseAudioCtx(cfg, nSamples, acftModel);
    WDEBUG("[Auris] Worker: audio_ctx %d\n", w.audio_ctx);
    return w;
}
