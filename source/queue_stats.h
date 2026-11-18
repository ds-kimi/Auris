// Backlog counters written by the job queue under its own lock
#pragma once
#include <cstddef>
#include <cstdint>

void QueueStatsReset();
// Called by the queue after every push/pop with the new total and new drops
void QueueStatsUpdate(size_t queuedSamples, uint64_t droppedNow);
uint64_t QueueStatsDropped();
size_t QueueStatsQueuedSamples();
