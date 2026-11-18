#include "transcribe/onnx/onnx_pool.h"
#include "transcribe/onnx/onnx_backend.h"
#include "transcribe/pool_limits.h"
#include "transcribe/worker_pool.h"

// The Silero gate still runs through whisper.cpp's VAD, which needs no model ctx
bool StartOnnxPool(const WhisperConfig& cfg) {
    if (!OnnxInit(cfg)) return false;
    if (StartWorkerPool(nullptr, ResolveWorkerCount(cfg), false, cfg.vad_model)) return true;
    OnnxShutdown();
    return false;
}
