// Runs clips through N workers sharing one whisper_context, like the module pool
#pragma once
#include "bench_options.h"
#include "whisper.h"
#include <string>
#include <vector>

struct BenchClip {
    std::string name;
    std::vector<float> audio;
};

struct ClipResult {
    std::string name;
    double audioSec = 0.0;
    double wallMs = 0.0;
    double cpuSec = 0.0;   // process CPU delta: exact per clip only with one worker
    bool skipped = false;  // Silero found no speech
    std::string text;
};

std::vector<ClipResult> RunBench(whisper_context* ctx, const BenchOptions& opts,
                                 const std::vector<BenchClip>& clips);
