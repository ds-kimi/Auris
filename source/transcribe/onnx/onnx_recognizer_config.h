// Fills the sherpa-onnx offline recognizer config for one model type
#pragma once
#ifdef AURIS_WITH_ONNX
#include "core/auris_config.h"
#include "transcribe/onnx/onnx_model_paths.h"
#include "sherpa-onnx/c-api/c-api.h"

// rc points into paths: keep paths alive until the recognizer is created
void FillRecognizerConfig(const WhisperConfig& cfg, const OnnxModelPaths& paths,
                          SherpaOnnxOfflineRecognizerConfig& rc);
#endif
