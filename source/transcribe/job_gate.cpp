// Energy gate and Silero crop, shared by the whisper and ONNX backends
#include "transcribe/job_gate.h"
#include "audio/audio_cache.h"
#include "util/debug_log.h"
#include "vad/silero_gate.h"
#include "vad/vad.h"

// Skips chunks whose trailing 1 s is as loud as the average, which covers both
// pure-silence buffers and continuous-noise buffers that would otherwise trigger
// whisper.cpp hallucinations ("Thanks for watching", "[Music]", etc.).
// vad_thold <= 0 disables the gate entirely.
static bool PassesVad(const WhisperConfig& cfg, const std::vector<float>& audio) {
    if (cfg.vad_thold <= 0.0f) return true;
    std::vector<float> vadCopy = audio;
    return auris::vad_simple(vadCopy, 16000, 1000, cfg.vad_thold, cfg.freq_thold, false);
}

// The energy gate runs first: it is far cheaper than the Silero LSTM
bool GateJobAudio(const WhisperConfig& cfg, const WorkerSlot& slot, TranscriptJob& job, float dur) {
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
