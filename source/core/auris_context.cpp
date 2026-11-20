// Auris model lifecycle: weights loaded once, transcribed by a worker pool
#include "core/auris_context.h"
#include "transcribe/asr_backend.h"
#include "core/auris_config.h"
#include "platform/cpu_features.h"
#include "transcribe/job_queue.h"
#include "transcribe/onnx/onnx_backend.h"
#include "transcribe/onnx/onnx_pool.h"
#include "transcribe/pool_limits.h"
#include "transcribe/result_queue.h"
#include "transcribe/worker_pool.h"
#include <cstdio>

static whisper_context* g_ctx = nullptr;
// Covers both backends; the ONNX path never sets g_ctx
static bool g_ready = false;

bool InitWhisper(const std::string& modelPath) {
    // A second Init() in the same Lua state (lua_refresh, addon reload) would
    // leak the model and re-start a worker pool that is still running
    if (g_ready) return true;
    if (!CpuHasRequiredFeatures()) {
        printf("[Auris] ERROR: CPU lacks AVX2/FMA/F16C, local backend disabled (use the OpenAI backend)\n");
        return false;
    }

    WhisperConfig cfg = GetWhisperConfig();
    if (ResolveAsrBackend(cfg) == AsrBackend::Onnx) return g_ready = StartOnnxPool(cfg);

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
    g_ready = true;
    return true;
}

void ShutdownWhisper() {
    g_ready = false;
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
