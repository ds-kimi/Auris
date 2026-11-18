// Sequential sherpa-onnx pass over the clips, for a first whisper comparison
#pragma once
#include "bench_run.h"

// Empty when the recognizer cannot load (32-bit build, missing files)
std::vector<ClipResult> RunOnnxBench(const BenchOptions& opts, const std::vector<BenchClip>& clips);
