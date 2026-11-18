// Auris model lifecycle and background transcription
#include "auris_context.h"
#include "auris_config.h"
#include "audio_cache.h"
#include "cpu_features.h"
#include <cstdio>
#include "debug_log.h"
#include "vad.h"

static whisper_context* g_ctx = nullptr;
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
        whisper_full_params wparams = whisper_full_default_params(
            cfg.use_beam_search
                ? WHISPER_SAMPLING_BEAM_SEARCH
                : WHISPER_SAMPLING_GREEDY
        );

        // core
        wparams.print_progress   = cfg.print_progress;
        wparams.print_timestamps = cfg.print_timestamps;
        wparams.single_segment   = cfg.single_segment;
        wparams.no_context       = cfg.no_context;
        wparams.language         = cfg.language.empty() ? nullptr : cfg.language.c_str();
        wparams.n_threads        = cfg.n_threads;

        // sampling
        wparams.greedy.best_of        = cfg.greedy_best_of;
        wparams.beam_search.beam_size = cfg.beam_size;

        // output
        wparams.translate         = cfg.translate;
        wparams.detect_language   = cfg.detect_language;
        wparams.no_timestamps     = cfg.no_timestamps;
        wparams.print_special     = cfg.print_special;
        wparams.print_realtime    = cfg.print_realtime;
        wparams.debug_mode        = cfg.debug_mode;
        wparams.tdrz_enable       = cfg.tdrz_enable;

        // token timestamps
        wparams.token_timestamps  = cfg.token_timestamps;
        wparams.thold_pt          = cfg.thold_pt;
        wparams.thold_ptsum       = cfg.thold_ptsum;
        wparams.max_len           = cfg.max_len;
        wparams.split_on_word     = cfg.split_on_word;
        wparams.max_tokens        = cfg.max_tokens;

        // filtering
        wparams.suppress_blank        = cfg.suppress_blank;
        wparams.suppress_nst          = cfg.suppress_nst;
        wparams.no_speech_thold       = cfg.no_speech_thold;
        wparams.suppress_regex        = cfg.suppress_regex.empty() ? nullptr : cfg.suppress_regex.c_str();
        wparams.initial_prompt        = cfg.initial_prompt.empty() ? nullptr : cfg.initial_prompt.c_str();
        wparams.carry_initial_prompt  = cfg.carry_initial_prompt;

        // decoding
        wparams.temperature     = cfg.temperature;
        wparams.temperature_inc = cfg.temperature_inc;
        wparams.entropy_thold   = cfg.entropy_thold;
        wparams.logprob_thold   = cfg.logprob_thold;
        wparams.max_initial_ts  = cfg.max_initial_ts;
        wparams.length_penalty  = cfg.length_penalty;

        // context
        wparams.n_max_text_ctx = cfg.n_max_text_ctx;
        wparams.offset_ms      = cfg.offset_ms;
        wparams.duration_ms    = cfg.duration_ms;
        wparams.audio_ctx      = cfg.audio_ctx;

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

        int ret = whisper_full(
            g_ctx, wparams,
            job.audio.data(), (int)job.audio.size()
        );
        if (ret != 0) {
            WDEBUG("[Auris] Worker: transcription failed\n");
            continue;
        }

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
