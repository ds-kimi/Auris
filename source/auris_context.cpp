// Auris model lifecycle and background transcription
#include "auris_context.h"
#include "auris_config.h"
#include "audio_cache.h"
#include "cpu_features.h"
#include <cstdio>
#include "debug_log.h"
#include "vad.h"
#include "whisper_params.h"
#include <algorithm>
#include <cctype>
#include <chrono>

static whisper_context* g_ctx = nullptr;
static bool g_acftModel = false;
static std::mutex g_resultMutex;
static std::queue<TranscriptResult> g_results;

static std::mutex g_jobMutex;
static std::condition_variable g_jobCond;
static std::queue<TranscriptJob> g_jobs;
static std::thread g_worker;
static std::atomic<bool> g_running(false);

static void WorkerLoop() {
    while (g_running) {
        TranscriptJob job;
        {
            std::unique_lock<std::mutex> lock(g_jobMutex);
            g_jobCond.wait(lock, [] {
                return !g_jobs.empty() || !g_running;
            });
            if (!g_running) break;
            job = std::move(g_jobs.front());
            g_jobs.pop();
        }

        if (job.audio.empty() || !g_ctx) continue;

        float dur = (float)job.audio.size() / 16000.0f;
        WDEBUG("[Auris] Worker: transcribing %.1fs\n", dur);

        WhisperConfig cfg = GetWhisperConfig();
        whisper_full_params wparams = BuildWhisperParams(cfg, (int)job.audio.size(), g_acftModel);

        // VAD gate: skip chunks whose trailing 1 s is as loud as the average,
        // which covers both pure-silence buffers and continuous-noise buffers
        // that would otherwise trigger whisper.cpp hallucinations
        // ("Thanks for watching", "[Music]", etc.).
        // vad_thold <= 0 disables the gate entirely.
        if (cfg.vad_thold > 0.0f) {
            std::vector<float> vad_copy = job.audio;
            if (!auris::vad_simple(vad_copy, 16000, 1000,
                                   cfg.vad_thold, cfg.freq_thold, false)) {
                WDEBUG("[Auris] VAD: silence, skipping %.1fs chunk\n", dur);
                continue;
            }
        }

        StoreCachedAudio(job.key, job.audio);

        auto t0 = std::chrono::steady_clock::now();
        int ret = whisper_full(
            g_ctx, wparams,
            job.audio.data(), (int)job.audio.size()
        );
        if (ret != 0) {
            WDEBUG("[Auris] Worker: transcription failed\n");
            continue;
        }

        // whisper.cpp exposes no fallback counter, and whisper_get_timings allocates and
        // averages per call; a real-time factor far above the usual one is the visible
        // sign of temperature fallbacks or a token loop
        double ms = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - t0).count();
        WDEBUG("[Auris] Timings: %.0f ms for %.1fs (rtf %.2f)\n", ms, dur, ms / 1000.0 / dur);

        int nSeg = whisper_full_n_segments(g_ctx);
        for (int i = 0; i < nSeg; i++) {
            const char* text = whisper_full_get_segment_text(g_ctx, i);
            if (!text || text[0] == '\0') continue;

            WDEBUG("[Auris] Result: %s\n", text);
            std::lock_guard<std::mutex> lock(g_resultMutex);
            g_results.push({job.key, std::string(text)});
        }
    }
}

bool InitWhisper(const std::string& modelPath) {
    if (!CpuHasRequiredFeatures()) {
        printf("[Auris] ERROR: CPU lacks AVX2/FMA/F16C, local backend disabled (use the OpenAI backend)\n");
        return false;
    }

    whisper_context_params cp = whisper_context_default_params();
    g_ctx = whisper_init_from_file_with_params(modelPath.c_str(), cp);
    if (!g_ctx) return false;
    std::string lower = modelPath;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return (char)std::tolower(c); });
    g_acftModel = lower.find("acft") != std::string::npos;
    printf("[Auris] %s\n", whisper_print_system_info());

    g_running = true;
    g_worker = std::thread(WorkerLoop);
    WDEBUG("[Auris] Worker thread started\n");
    return true;
}

void ShutdownWhisper() {
    g_running = false;
    g_jobCond.notify_all();
    if (g_worker.joinable()) g_worker.join();

    if (g_ctx) {
        whisper_free(g_ctx);
        g_ctx = nullptr;
    }
}

void QueueTranscription(int key, std::vector<float> audio) {
    std::lock_guard<std::mutex> lock(g_jobMutex);
    g_jobs.push({key, std::move(audio)});
    g_jobCond.notify_one();
}

bool PollResult(TranscriptResult& out) {
    std::lock_guard<std::mutex> lock(g_resultMutex);
    if (g_results.empty()) return false;
    out = std::move(g_results.front());
    g_results.pop();
    return true;
}
