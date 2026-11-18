// One job end to end: silence gate, whisper_full_with_state, segment collection
#include "worker_job.h"
#include "audio_cache.h"
#include "debug_log.h"
#include "result_queue.h"
#include "silero_gate.h"
#include "silero_gate.h"
#include "vad.h"
#include "whisper_params.h"
#include <chrono>

// Skips chunks whose trailing 1 s is as loud as the average, which covers both
// pure-silence buffers and continuous-noise buffers that would otherwise trigger
// whisper.cpp hallucinations ("Thanks for watching", "[Music]", etc.).
// vad_thold <= 0 disables the gate entirely.
static bool PassesVad(const WhisperConfig& cfg, const std::vector<float>& audio) {
    if (cfg.vad_thold <= 0.0f) return true;
    std::vector<float> vadCopy = audio;
    return auris::vad_simple(vadCopy, 16000, 1000, cfg.vad_thold, cfg.freq_thold, false);
}

static void CollectSegments(whisper_state* state, int key) {
    int nSeg = whisper_full_n_segments_from_state(state);
    for (int i = 0; i < nSeg; i++) {
        const char* text = whisper_full_get_segment_text_from_state(state, i);
        if (!text || text[0] == '\0') continue;
        WDEBUG("[Auris] Result: %s\n", text);
        PushResult({key, std::string(text)});
    }
}

// The energy gate runs first: it is far cheaper than the Silero LSTM
static bool GateAudio(const WhisperConfig& cfg, const WorkerSlot& slot,
                      TranscriptJob& job, float dur) {
    if (!PassesVad(cfg, job.audio)) {
        WDEBUG("[Auris] VAD: silence, skipping %.1fs chunk\n", dur);
        return false;
    }
    // the cache keeps the uncropped clip: it is what the player actually said
    StoreCachedAudio(job.key, job.audio);
    if (slot.vad && !SileroCropToSpeech(slot.vad, job.audio)) {
        WDEBUG("[Auris] Silero: no speech, skipping %.1fs chunk\n", dur);
        return false;
    }
    return true;
}

void RunTranscriptionJob(whisper_context* ctx, const WorkerSlot& slot,
                         TranscriptJob& job, bool acftModel) {
    float dur = (float)job.audio.size() / 16000.0f;
    WDEBUG("[Auris] Worker: transcribing %.1fs\n", dur);

    WhisperConfig cfg = GetWhisperConfig();
    if (!GateAudio(cfg, slot, job, dur)) return;

    int n = (int)job.audio.size();
    whisper_full_params wparams = BuildWhisperParams(cfg, n, acftModel);
    auto t0 = std::chrono::steady_clock::now();
    if (whisper_full_with_state(ctx, slot.state, wparams, job.audio.data(), n) != 0) {
        WDEBUG("[Auris] Worker: transcription failed\n");
        return;
    }
    // whisper.cpp exposes no fallback counter, and whisper_get_timings allocates and
    // averages per call; a real-time factor far above the usual one is the visible
    // sign of temperature fallbacks or a token loop
    double ms = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - t0).count();
    float run = (float)n / 16000.0f;
    WDEBUG("[Auris] Timings: %.0f ms for %.1fs (rtf %.2f)\n", ms, run, ms / 1000.0 / run);
    CollectSegments(slot.state, job.key);
}
