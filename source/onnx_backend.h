// sherpa-onnx offline recognizer (Moonshine, Parakeet, ...) shared by all workers
#pragma once
#include "auris_config.h"
#include <string>
#include <vector>

// Loads the model named by cfg.onnx_model_type / onnx_model_dir; false on a
// 32-bit build or when the files are missing
bool OnnxInit(const WhisperConfig& cfg);
void OnnxShutdown();
// One stream per call, so concurrent workers never share decoder state
bool OnnxTranscribe(const std::vector<float>& audio, std::string& text);
