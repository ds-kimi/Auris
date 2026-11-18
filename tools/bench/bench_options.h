// Command line of auris-bench
#pragma once
#include <string>
#include <vector>

struct BenchOptions {
    std::string model;
    std::string wavPath;         // a .wav file or a directory of them
    std::string vadModel;        // empty = Silero gate off
    std::string language = "en";
    std::string onnxDir;         // set = sherpa-onnx instead of -m (64-bit only)
    std::string onnxType = "moonshine"; // moonshine | parakeet | zipformer
    int workers = 1;
    int threads = 1;             // per worker, like WhisperConfig::n_threads
    int repeat = 1;              // each clip is queued this many times
    bool dynamicCtx = true;
};

// False (after printing usage) on missing or malformed arguments
bool ParseBenchArgs(int argc, char** argv, BenchOptions& opts);
// The file itself, or every *.wav directly inside the directory, sorted
std::vector<std::string> ListWavFiles(const std::string& path);
