// Whisper model lifecycle and background transcription
#include "whisper_context.h"
#include "whisper_config.h"
#include "debug_log.h"

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
        WDEBUG("[Whisper] Worker: transcribing %.1fs\n", dur);

        WhisperConfig cfg = GetWhisperConfig();
        whisper_full_params wparams =
            whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
        wparams.print_progress = cfg.print_progress;
        wparams.print_timestamps = cfg.print_timestamps;
        wparams.single_segment = cfg.single_segment;
        wparams.no_context = cfg.no_context;
        wparams.language = cfg.language.c_str();
        wparams.n_threads = cfg.n_threads;

        int ret = whisper_full(
            g_ctx, wparams,
            job.audio.data(), (int)job.audio.size()
        );
        if (ret != 0) {
            WDEBUG("[Whisper] Worker: whisper_full failed\n");
            continue;
        }

        int nSeg = whisper_full_n_segments(g_ctx);
        for (int i = 0; i < nSeg; i++) {
            const char* text =
                whisper_full_get_segment_text(g_ctx, i);
            if (!text || text[0] == '\0') continue;

            WDEBUG("[Whisper] Result: %s\n", text);
            std::lock_guard<std::mutex> lock(g_resultMutex);
            g_results.push({job.key, std::string(text)});
        }
    }
}

bool InitWhisper(const std::string& modelPath) {
    whisper_context_params cp = whisper_context_default_params();
    g_ctx = whisper_init_from_file_with_params(
        modelPath.c_str(), cp
    );
    if (!g_ctx) return false;

    g_running = true;
    g_worker = std::thread(WorkerLoop);
    WDEBUG("[Whisper] Worker thread started\n");
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
