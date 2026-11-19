// Workers write their own slot only and the main thread reads with relaxed
// atomics: a GetStats call must never wait behind a running decode.
#include "transcribe/worker_stats.h"
#include "transcribe/queue_stats.h"
#include "transcribe/worker_tuner.h"
#include <atomic>

static const int kMaxWorkers = 8;

struct WorkerCounters {
    std::atomic<uint64_t> jobs{0};
    std::atomic<uint64_t> cpuMicros{0};
    std::atomic<uint64_t> samples{0};
    std::atomic<int> core{-1};
};

static WorkerCounters g_counters[kMaxWorkers];
static std::atomic<int> g_workers{0};

void StatsReset(int workers) {
    for (WorkerCounters& c : g_counters) {
        c.jobs = 0;
        c.cpuMicros = 0;
        c.samples = 0;
        c.core = -1;
    }
    g_workers = workers < kMaxWorkers ? workers : kMaxWorkers;
    QueueStatsReset();
}

void StatsRecordJob(int worker, double cpuSeconds, size_t samples, int core) {
    if (worker < 0 || worker >= kMaxWorkers) return;
    WorkerCounters& c = g_counters[worker];
    c.jobs.fetch_add(1, std::memory_order_relaxed);
    c.cpuMicros.fetch_add((uint64_t)(cpuSeconds * 1e6), std::memory_order_relaxed);
    c.samples.fetch_add(samples, std::memory_order_relaxed);
    c.core.store(core, std::memory_order_relaxed);
}

PoolStatsView StatsSnapshot() {
    PoolStatsView view;
    view.dropped = QueueStatsDropped();
    view.queueSeconds = (double)QueueStatsQueuedSamples() / 16000.0;
    view.activeWorkers = TunerActiveWorkers();
    for (int i = 0; i < g_workers.load(); i++) {
        WorkerStatsView w;
        w.jobs = g_counters[i].jobs.load(std::memory_order_relaxed);
        w.cpuSeconds = g_counters[i].cpuMicros.load(std::memory_order_relaxed) / 1e6;
        w.audioSeconds = g_counters[i].samples.load(std::memory_order_relaxed) / 16000.0;
        w.core = g_counters[i].core.load(std::memory_order_relaxed);
        view.workers.push_back(w);
    }
    return view;
}
