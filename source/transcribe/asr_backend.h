// Which speech recognizer the worker pool runs
#pragma once
#include "core/auris_config.h"

enum class AsrBackend { Whisper, Onnx };

// cfg.backend, falling back to whisper (with a console error) when the build
// has no ONNX runtime or the value is unknown
AsrBackend ResolveAsrBackend(const WhisperConfig& cfg);
