// whisper_context holds only the weights and is read-only during inference, so
// workers share it; everything mutated by a decode lives in a per-worker state.
#include "worker_pool.h"
#include "debug_log.h"
#include "job_queue.h"
#include "silero_gate.h"
#include "thread_priority.h"
#include "worker_job.h"
#include <thread>
#include <vector>

static whisper_context* g_poolCtx = nullptr;
static bool g_poolAcft = false;
static std::vector<WorkerSlot> g_slots;
static std::vector<std::thread> g_threads;

static void WorkerMain(WorkerSlot slot) {
    LowerCurrentThreadPriority();
    TranscriptJob job;
    while (JobQueuePop(job)) {
        if (job.audio.empty()) continue;
        RunTranscriptionJob(g_poolCtx, slot, job, g_poolAcft);
    }
}

static void FreeSlots() {
    for (WorkerSlot& slot : g_slots) {
        if (slot.state) whisper_free_state(slot.state);
        if (slot.vad) whisper_vad_free(slot.vad);
    }
    g_slots.clear();
}

bool StartWorkerPool(whisper_context* ctx, int workers, bool acftModel,
                     const std::string& vadPath) {
    g_poolCtx = ctx;
    g_poolAcft = acftModel;
    bool useVad = SileroModelAvailable(vadPath);
    for (int i = 0; i < workers; i++) {
        WorkerSlot slot;
        // the ONNX backend (ctx null) keeps its decoder state per stream instead
        if (ctx) slot.state = whisper_init_state(ctx);
        if (ctx && !slot.state) {
            printf("[Auris] ERROR: could not allocate whisper state %d/%d\n", i + 1, workers);
            FreeSlots();
            return false;
        }
        // a corrupt VAD file only costs the gate, never the transcription
        if (useVad) slot.vad = InitSileroGate(vadPath);
        g_slots.push_back(slot);
    }
    JobQueueOpen();
    for (const WorkerSlot& slot : g_slots) g_threads.emplace_back(WorkerMain, slot);
    WDEBUG("[Auris] %d worker thread(s) started\n", workers);
    return true;
}

void StopWorkerPool() {
    JobQueueClose();
    for (std::thread& t : g_threads) {
        if (t.joinable()) t.join();
    }
    g_threads.clear();
    FreeSlots();
    g_poolCtx = nullptr;
}
