// N transcription threads sharing one set of model weights
#pragma once
#include "whisper.h"

// Allocates one whisper_state per worker; false (nothing started) if any fails
bool StartWorkerPool(whisper_context* ctx, int workers, bool acftModel);
// Joins every worker, then frees their states; the ctx stays with the caller
void StopWorkerPool();
