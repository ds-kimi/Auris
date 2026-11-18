// Worker pool start-up for the ONNX backend
#pragma once
#include "auris_config.h"

// Loads the shared recognizer, then starts the workers without whisper states
bool StartOnnxPool(const WhisperConfig& cfg);
