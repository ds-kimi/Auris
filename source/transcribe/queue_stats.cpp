#include "transcribe/queue_stats.h"
#include <atomic>

static std::atomic<size_t> g_queued{0};
static std::atomic<uint64_t> g_dropped{0};

void QueueStatsReset() {
    g_queued = 0;
    g_dropped = 0;
}

void QueueStatsUpdate(size_t queuedSamples, uint64_t droppedNow) {
    g_queued.store(queuedSamples, std::memory_order_relaxed);
    if (droppedNow) g_dropped.fetch_add(droppedNow, std::memory_order_relaxed);
}

uint64_t QueueStatsDropped() {
    return g_dropped.load(std::memory_order_relaxed);
}

size_t QueueStatsQueuedSamples() {
    return g_queued.load(std::memory_order_relaxed);
}
