// Auris model lifecycle: weights loaded once, transcribed by a worker pool
#include "auris_context.h"
#include "asr_backend.h"
#include "auris_config.h"
#include "cpu_features.h"
#include "job_queue.h"
#include "onnx_backend.h"
#include "onnx_pool.h"
#include "pool_limits.h"
#include "result_queue.h"
#include "worker_pool.h"
#include <cstdio>

static whisper_context* g_ctx = nullptr;

bool InitWhisper(const std::string& modelPath) {
    if (!CpuHasRequiredFeatures()) {
        printf("[Auris] ERROR: CPU lacks AVX2/FMA/F16C, local backend disabled (use the OpenAI backend)\n");
        return false;
    }

    WhisperConfig cfg = GetWhisperConfig();
    if (ResolveAsrBackend(cfg) == AsrBackend::Onnx) return StartOnnxPool(cfg);

    // no_state: the default state would be one more KV cache nobody decodes with
    whisper_context_params cp = whisper_context_default_params();
    g_ctx = whisper_init_from_file_with_params_no_state(modelPath.c_str(), cp);
    if (!g_ctx) return false;
    printf("[Auris] %s\n", whisper_print_system_info());

    int workers = ResolveWorkerCount(cfg);
    if (!StartWorkerPool(g_ctx, workers, IsAcftModel(modelPath), cfg.vad_model)) {
        whisper_free(g_ctx);
        g_ctx = nullptr;
        return false;
    }
    return true;
}

void ShutdownWhisper() {
    StopWorkerPool();
    OnnxShutdown();
    if (g_ctx) {
        whisper_free(g_ctx);
        g_ctx = nullptr;
    }
}

void QueueTranscription(int key, std::vector<float> audio) {
    JobQueuePush({key, std::move(audio)}, MaxQueueSamples(GetWhisperConfig()));
}

bool PollResult(TranscriptResult& out) {
    return PopResult(out);
}
