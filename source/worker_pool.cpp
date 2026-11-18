// whisper_context holds only the weights and is read-only during inference, so
// workers share it; everything mutated by a decode lives in a per-worker state.
#include "worker_pool.h"
#include "auris_config.h"
#include "debug_log.h"
#include "job_queue.h"
#include "silero_gate.h"
#include "worker_loop.h"
#include "worker_placement.h"
#include "worker_stats.h"
#include "worker_tuner.h"
#include <thread>
#include <vector>

static std::vector<WorkerSlot> g_slots;
static std::vector<std::thread> g_threads;

static void FreeSlots() {
    for (WorkerSlot& slot : g_slots) {
        if (slot.state) whisper_free_state(slot.state);
        if (slot.vad) whisper_vad_free(slot.vad);
    }
    g_slots.clear();
}

// whisper_init_state reserves every ggml compute buffer once; each
// whisper_full_with_state reuses them, so no per-job model allocation remains
static bool AllocateSlots(whisper_context* ctx, int workers, const std::string& vadPath) {
    bool useVad = SileroModelAvailable(vadPath);
    for (int i = 0; i < workers; i++) {
        WorkerSlot slot;
        slot.index = i;
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
    return true;
}

bool StartWorkerPool(whisper_context* ctx, int workers, bool acftModel,
                     const std::string& vadPath) {
    if (!AllocateSlots(ctx, workers, vadPath)) return false;
    WhisperConfig cfg = GetWhisperConfig();
    StatsReset(workers);
    TunerReset(workers, cfg.auto_workers);
    // before the threads exist, so the first mask is ready when they ask
    ConfigurePlacement(cfg);
    JobQueueOpen();
    SetWorkerLoopRunning(true);
    for (const WorkerSlot& slot : g_slots) g_threads.emplace_back(WorkerMain, ctx, slot, acftModel);
    WDEBUG("[Auris] %d worker thread(s) started\n", workers);
    return true;
}

void StopWorkerPool() {
    SetWorkerLoopRunning(false);
    JobQueueClose();
    for (std::thread& t : g_threads) {
        if (t.joinable()) t.join();
    }
    g_threads.clear();
    FreeSlots();
}
