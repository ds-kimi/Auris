// auris-bench: measures Auris transcription cost outside of srcds
#include "bench_onnx.h"
#include "bench_options.h"
#include "bench_run.h"
#include "bench_stats.h"
#include "wav_reader.h"
#include <chrono>
#include <cstdio>

static std::vector<BenchClip> LoadClips(const std::string& path) {
    std::vector<BenchClip> clips;
    for (const std::string& file : ListWavFiles(path)) {
        BenchClip c;
        c.name = file;
        if (ReadWav16kMono(file, c.audio)) clips.push_back(std::move(c));
        else printf("skip %s: not a 16 kHz mono PCM16/float32 wav\n", file.c_str());
    }
    return clips;
}

static void PrintReport(const std::vector<ClipResult>& results, double wallSec, double cpuSec) {
    std::vector<double> walls;
    double audio = 0.0;
    int skipped = 0;
    for (const ClipResult& r : results) {
        printf("%-40s %6.2fs %8.0f ms cpu/audio %.3f%s |%s\n", r.name.c_str(), r.audioSec, r.wallMs,
               r.audioSec > 0 ? r.cpuSec / r.audioSec : 0.0, r.skipped ? " [vad skip]" : "", r.text.c_str());
        walls.push_back(r.wallMs);
        audio += r.audioSec;
        skipped += r.skipped ? 1 : 0;
    }
    printf("\nclips %zu (vad skipped %d), audio %.1fs, wall %.2fs, throughput %.2f audio-s/s\n",
           results.size(), skipped, audio, wallSec, wallSec > 0 ? audio / wallSec : 0.0);
    printf("latency p50 %.0f ms, p95 %.0f ms\n", Percentile(walls, 0.5), Percentile(walls, 0.95));
    printf("total CPU %.2fs, CPU-s per audio-s %.3f, peak RSS %.0f MiB\n",
           cpuSec, audio > 0 ? cpuSec / audio : 0.0, PeakRssMiB());
    printf("fallbacks: n/a (whisper.cpp exposes no counter; watch for outlier wall ms)\n");
}

int main(int argc, char** argv) {
    BenchOptions opts;
    if (!ParseBenchArgs(argc, argv, opts)) return 1;
    std::vector<BenchClip> clips = LoadClips(opts.wavPath);
    if (clips.empty()) return 1;
    if (!opts.onnxDir.empty()) {
        double cpu0 = ProcessCpuSeconds();
        auto t0 = std::chrono::steady_clock::now();
        std::vector<ClipResult> results = RunOnnxBench(opts, clips);
        double wall = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        if (results.empty()) return 1;
        PrintReport(results, wall, ProcessCpuSeconds() - cpu0);
        return 0;
    }
    whisper_context_params cp = whisper_context_default_params();
    cp.use_gpu = false;
    whisper_context* ctx = whisper_init_from_file_with_params_no_state(opts.model.c_str(), cp);
    if (!ctx) return 1;
    double cpu0 = ProcessCpuSeconds();
    auto t0 = std::chrono::steady_clock::now();
    std::vector<ClipResult> results = RunBench(ctx, opts, clips);
    double wall = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    PrintReport(results, wall, ProcessCpuSeconds() - cpu0);
    whisper_free(ctx);
    return 0;
}
