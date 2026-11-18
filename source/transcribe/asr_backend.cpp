// Backend choice is read once at Init: the pool is built for one engine
#include "asr_backend.h"
#include <cstdio>

AsrBackend ResolveAsrBackend(const WhisperConfig& cfg) {
    if (cfg.backend != "onnx") {
        if (cfg.backend != "whisper") {
            printf("[Auris] ERROR: unknown backend '%s', using whisper\n", cfg.backend.c_str());
        }
        return AsrBackend::Whisper;
    }
#ifdef AURIS_WITH_ONNX
    return AsrBackend::Onnx;
#else
    // ONNX Runtime ships no 32-bit Linux build, so 32-bit modules are whisper only
    printf("[Auris] ERROR: backend 'onnx' needs a 64-bit build, using whisper\n");
    return AsrBackend::Whisper;
#endif
}
