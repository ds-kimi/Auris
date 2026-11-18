// Body of one transcription worker thread
#pragma once
#include "transcribe/worker_job.h"

// Runs until SetWorkerLoopRunning(false) and the queue closes; ctx null = ONNX
void WorkerMain(whisper_context* ctx, WorkerSlot slot, bool acftModel);
// Set true before the threads start, false before the queue closes
void SetWorkerLoopRunning(bool running);
