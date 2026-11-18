// Placement and accounting wrap every job here so both backends (whisper and
// ONNX) get them without either decode path knowing.
#include "transcribe/worker_loop.h"
#include "transcribe/job_queue.h"
#include "platform/thread_cpu.h"
#include "platform/thread_priority.h"
#include "transcribe/worker_placement.h"
#include "transcribe/worker_stats.h"
#include "transcribe/worker_tuner.h"
#include <atomic>
#include <chrono>
#include <thread>

static std::atomic<bool> g_running{false};

// the clip length is taken before the Silero gate crops it: the cost that
// matters is CPU per second of voice the server received
static void RunAccountedJob(whisper_context* ctx, const WorkerSlot& slot,
                            TranscriptJob& job, bool acftModel) {
    double waited = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - job.queuedAt).count();
    TunerObserve(waited);
    size_t samples = job.audio.size();
    double cpu0 = ThreadCpuSeconds();
    RunTranscriptionJob(ctx, slot, job, acftModel);
    StatsRecordJob(slot.index, ThreadCpuSeconds() - cpu0, samples, CurrentCore());
}

void WorkerMain(whisper_context* ctx, WorkerSlot slot, bool acftModel) {
    LowerCurrentThreadPriority();
    uint32_t placement = 0;
    TranscriptJob job;
    while (g_running.load()) {
        ApplyWorkerPlacement(placement);
        // a parked worker polls instead of waiting on the queue so a raise
        // from the tuner is picked up without a dedicated wake-up path
        if (!TunerWorkerActive(slot.index)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        if (!JobQueuePop(job)) break;
        if (job.audio.empty()) continue;
        RunAccountedJob(ctx, slot, job, acftModel);
    }
}

void SetWorkerLoopRunning(bool running) {
    g_running.store(running);
}
