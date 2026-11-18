// Bounded FIFO of clips waiting for a transcription worker
#pragma once
#include "auris_context.h"
#include <cstddef>

// Pushes a job, then drops the oldest ones while more than maxSamples are queued
void JobQueuePush(TranscriptJob job, size_t maxSamples);
// Blocks until a job is available or the queue is closed; false once closed
bool JobQueuePop(TranscriptJob& out);
// Reopens the queue for a new pool, discarding stale jobs
void JobQueueOpen();
// Wakes every waiting worker so it can exit
void JobQueueClose();
