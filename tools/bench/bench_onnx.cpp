// One clip at a time: per-clip CPU stays exact; worker scaling comes with J2
#include "bench_onnx.h"
#include "bench_stats.h"
#include "transcribe/onnx/onnx_backend.h"
#include <chrono>

static ClipResult RunOnnxClip(const BenchClip& clip) {
    ClipResult r;
    r.name = clip.name;
    r.audioSec = (double)clip.audio.size() / 16000.0;
    double cpu0 = ProcessCpuSeconds();
    auto t0 = std::chrono::steady_clock::now();
    OnnxTranscribe(clip.audio, r.text);
    r.wallMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    r.cpuSec = ProcessCpuSeconds() - cpu0;
    return r;
}

std::vector<ClipResult> RunOnnxBench(const BenchOptions& opts, const std::vector<BenchClip>& clips) {
    WhisperConfig cfg;
    cfg.n_threads = opts.threads;
    cfg.onnx_model_type = opts.onnxType;
    cfg.onnx_model_dir = opts.onnxDir;
    std::vector<ClipResult> results;
    if (!OnnxInit(cfg)) return results;
    for (int rep = 0; rep < opts.repeat; ++rep) {
        for (const BenchClip& clip : clips) results.push_back(RunOnnxClip(clip));
    }
    OnnxShutdown();
    return results;
}
