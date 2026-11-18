// N transcription threads sharing one set of model weights
#pragma once
#include "whisper.h"
#include <string>

// Allocates one whisper_state (and a Silero context if vadPath exists) per
// worker; false (nothing started) if any state fails. ctx null = ONNX backend
bool StartWorkerPool(whisper_context* ctx, int workers, bool acftModel,
                     const std::string& vadPath);
// Joins every worker, then frees their states; the ctx stays with the caller
void StopWorkerPool();
