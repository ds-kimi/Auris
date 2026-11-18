// One job end to end: silence gate, decode on the pool's backend, result push
#include "worker_job.h"
#include "debug_log.h"
#include "job_gate.h"
#include "onnx_backend.h"
#include "result_queue.h"
#include "whisper_params.h"
#include <chrono>

static void CollectSegments(whisper_state* state, int key) {
    int nSeg = whisper_full_n_segments_from_state(state);
    for (int i = 0; i < nSeg; i++) {
        const char* text = whisper_full_get_segment_text_from_state(state, i);
        if (!text || text[0] == '\0') continue;
        WDEBUG("[Auris] Result: %s\n", text);
        PushResult({key, std::string(text)});
    }
}

static bool RunWhisper(whisper_context* ctx, const WorkerSlot& slot, TranscriptJob& job,
                       const WhisperConfig& cfg, bool acftModel) {
    int n = (int)job.audio.size();
    whisper_full_params wparams = BuildWhisperParams(cfg, n, acftModel);
    if (whisper_full_with_state(ctx, slot.state, wparams, job.audio.data(), n) != 0) return false;
    CollectSegments(slot.state, job.key);
    return true;
}

static bool RunOnnx(TranscriptJob& job) {
    std::string text;
    if (!OnnxTranscribe(job.audio, text)) return false;
    if (text.empty()) return true;
    WDEBUG("[Auris] Result: %s\n", text.c_str());
    PushResult({job.key, text});
    return true;
}

void RunTranscriptionJob(whisper_context* ctx, const WorkerSlot& slot,
                         TranscriptJob& job, bool acftModel) {
    float dur = (float)job.audio.size() / 16000.0f;
    WDEBUG("[Auris] Worker: transcribing %.1fs\n", dur);

    WhisperConfig cfg = GetWhisperConfig();
    if (!GateJobAudio(cfg, slot, job, dur)) return;

    auto t0 = std::chrono::steady_clock::now();
    bool ok = ctx ? RunWhisper(ctx, slot, job, cfg, acftModel) : RunOnnx(job);
    if (!ok) {
        WDEBUG("[Auris] Worker: transcription failed\n");
        return;
    }
    // whisper.cpp exposes no fallback counter; a real-time factor far above the
    // usual one is the visible sign of temperature fallbacks or a token loop
    double ms = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - t0).count();
    float run = (float)job.audio.size() / 16000.0f;
    WDEBUG("[Auris] Timings: %.0f ms for %.1fs (rtf %.2f)\n", ms, run, ms / 1000.0 / run);
}
