// sensevoice stays whitelisted in Lua but is rejected here: it was not
// benched, so operators get a clear error instead of an untested path
#include "onnx_model_paths.h"
#include "onnx_file_find.h"
#include <cstdio>
#include <fstream>

static bool FileExists(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    return !path.empty() && f.good();
}

// sherpa-onnx Moonshine v2 export: encoder + merged (cached/uncached) decoder
static void MoonshinePaths(const std::string& dir, OnnxModelPaths& out) {
    out.encoder = dir + "/encoder_model.ort";
    out.decoder = dir + "/decoder_model_merged.ort";
}

// Parakeet (NeMo) and Zipformer (icefall) share the transducer file trio
static void TransducerPaths(const std::string& dir, OnnxModelPaths& out) {
    out.encoder = FindOnnxFile(dir, "encoder");
    out.decoder = FindOnnxFile(dir, "decoder");
    out.joiner = FindOnnxFile(dir, "joiner");
}

bool ResolveOnnxModelPaths(const WhisperConfig& cfg, OnnxModelPaths& out) {
    const std::string& type = cfg.onnx_model_type;
    if (type == "moonshine") MoonshinePaths(cfg.onnx_model_dir, out);
    else if (type == "parakeet" || type == "zipformer") TransducerPaths(cfg.onnx_model_dir, out);
    else {
        printf("[Auris] ERROR: onnx_model_type '%s' is not supported by this build\n", type.c_str());
        return false;
    }
    out.tokens = cfg.onnx_model_dir + "/tokens.txt";
    for (const std::string* p : {&out.encoder, &out.decoder, &out.tokens}) {
        if (!FileExists(*p)) {
            printf("[Auris] ERROR: missing ONNX model file in %s\n", cfg.onnx_model_dir.c_str());
            return false;
        }
    }
    if (type != "moonshine" && !FileExists(out.joiner)) {
        printf("[Auris] ERROR: missing ONNX joiner in %s\n", cfg.onnx_model_dir.c_str());
        return false;
    }
    return true;
}
