// Model file layout of each whitelisted sherpa-onnx model type
#pragma once
#include "core/auris_config.h"
#include <string>

// Owns the strings the sherpa-onnx config points into
struct OnnxModelPaths {
    std::string encoder;
    std::string decoder;
    std::string joiner;          // transducers only (parakeet, zipformer)
    std::string tokens;
};

// false for a model type outside the whitelist or a missing model file
bool ResolveOnnxModelPaths(const WhisperConfig& cfg, OnnxModelPaths& out);
