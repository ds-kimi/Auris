#include "bench_run.h"
#include "bench_stats.h"
#include "pool_limits.h"
#include "silero_gate.h"
#include "whisper_params.h"
#include <atomic>
#include <chrono>
#include <thread>

static WhisperConfig BenchConfig(const BenchOptions& opts) {
    WhisperConfig cfg;
    cfg.language = opts.language;
    // config.lua ships no_context = true; without it one clip prompts the next
    cfg.no_context = true;
    cfg.n_threads = opts.threads;
    cfg.dynamic_audio_ctx = opts.dynamicCtx;
    return cfg;
}

static void TranscribeClip(whisper_context* ctx, whisper_state* state, whisper_vad_context* vad,
                           const BenchOptions& opts, const BenchClip& clip, ClipResult& r) {
    WhisperConfig cfg = BenchConfig(opts);
    std::vector<float> audio = clip.audio;
    r.name = clip.name;
    r.audioSec = audio.size() / 16000.0;
    double cpu0 = ProcessCpuSeconds();
    auto t0 = std::chrono::steady_clock::now();
    r.skipped = vad && !SileroCropToSpeech(vad, audio);
    if (!r.skipped) {
        int n = (int)audio.size();
        whisper_full_params w = BuildWhisperParams(cfg, n, IsAcftModel(opts.model));
        if (whisper_full_with_state(ctx, state, w, audio.data(), n) == 0) {
            for (int i = 0; i < whisper_full_n_segments_from_state(state); ++i)
                r.text += whisper_full_get_segment_text_from_state(state, i);
        }
    }
    r.wallMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    r.cpuSec = ProcessCpuSeconds() - cpu0;
}

// Each worker owns its whisper_state and Silero context, as in the module pool
static void WorkerLoop(whisper_context* ctx, const BenchOptions& opts, const std::vector<BenchClip>& clips,
                       std::vector<ClipResult>& results, std::atomic<size_t>& next) {
    whisper_state* state = whisper_init_state(ctx);
    if (!state) return;
    whisper_vad_context* vad = opts.vadModel.empty() ? nullptr : InitSileroGate(opts.vadModel);
    size_t total = clips.size() * (size_t)opts.repeat;
    for (size_t i = next++; i < total; i = next++) {
        TranscribeClip(ctx, state, vad, opts, clips[i % clips.size()], results[i]);
    }
    if (vad) whisper_vad_free(vad);
    whisper_free_state(state);
}

std::vector<ClipResult> RunBench(whisper_context* ctx, const BenchOptions& opts,
                                 const std::vector<BenchClip>& clips) {
    std::vector<ClipResult> results(clips.size() * (size_t)opts.repeat);
    std::atomic<size_t> next(0);
    std::vector<std::thread> threads;
    for (int i = 0; i < opts.workers; ++i)
        threads.emplace_back(WorkerLoop, ctx, std::cref(opts), std::cref(clips), std::ref(results), std::ref(next));
    for (auto& t : threads) t.join();
    return results;
}
