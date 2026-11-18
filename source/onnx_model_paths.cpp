// Only Moonshine is wired so far; the other whitelisted types are reserved so
// operators get a clear error instead of a silent fallback
#include "onnx_model_paths.h"
#include <cstdio>
#include <fstream>

static bool FileExists(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    return f.good();
}

// sherpa-onnx Moonshine v2 export: encoder + merged (cached/uncached) decoder
static void MoonshinePaths(const std::string& dir, OnnxModelPaths& out) {
    out.encoder = dir + "/encoder_model.ort";
    out.decoder = dir + "/decoder_model_merged.ort";
    out.tokens  = dir + "/tokens.txt";
}

bool ResolveOnnxModelPaths(const WhisperConfig& cfg, OnnxModelPaths& out) {
    if (cfg.onnx_model_type != "moonshine") {
        printf("[Auris] ERROR: onnx_model_type '%s' is not supported by this build\n",
               cfg.onnx_model_type.c_str());
        return false;
    }
    MoonshinePaths(cfg.onnx_model_dir, out);
    for (const std::string* p : {&out.encoder, &out.decoder, &out.tokens}) {
        if (!FileExists(*p)) {
            printf("[Auris] ERROR: missing ONNX model file %s\n", p->c_str());
            return false;
        }
    }
    return true;
}
