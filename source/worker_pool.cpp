// whisper_context holds only the weights and is read-only during inference, so
// workers share it; everything mutated by a decode lives in a per-worker state.
#include "worker_pool.h"
#include "debug_log.h"
#include "job_queue.h"
#include "thread_priority.h"
#include "worker_job.h"
#include <thread>
#include <vector>

static whisper_context* g_poolCtx = nullptr;
static bool g_poolAcft = false;
static std::vector<whisper_state*> g_states;
static std::vector<std::thread> g_threads;

static void WorkerMain(whisper_state* state) {
    LowerCurrentThreadPriority();
    TranscriptJob job;
    while (JobQueuePop(job)) {
        if (job.audio.empty()) continue;
        RunTranscriptionJob(g_poolCtx, state, job, g_poolAcft);
    }
}

static void FreeStates() {
    for (whisper_state* state : g_states) whisper_free_state(state);
    g_states.clear();
}

bool StartWorkerPool(whisper_context* ctx, int workers, bool acftModel) {
    g_poolCtx = ctx;
    g_poolAcft = acftModel;
    for (int i = 0; i < workers; i++) {
        whisper_state* state = whisper_init_state(ctx);
        if (!state) {
            printf("[Auris] ERROR: could not allocate whisper state %d/%d\n", i + 1, workers);
            FreeStates();
            return false;
        }
        g_states.push_back(state);
    }
    JobQueueOpen();
    for (whisper_state* state : g_states) g_threads.emplace_back(WorkerMain, state);
    WDEBUG("[Auris] %d worker thread(s) started\n", workers);
    return true;
}

void StopWorkerPool() {
    JobQueueClose();
    for (std::thread& t : g_threads) {
        if (t.joinable()) t.join();
    }
    g_threads.clear();
    FreeStates();
    g_poolCtx = nullptr;
}
