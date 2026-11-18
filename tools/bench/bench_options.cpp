#include "bench_options.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>

static void PrintUsage() {
    printf("usage: auris-bench -m <model.bin> -w <file.wav|dir> [-j workers] [-t threads]\n"
           "                   [-r repeat] [-l lang] [--vad <silero.bin>] [--no-dynamic-ctx]\n");
}

static bool ApplyArg(const char* key, const char* val, BenchOptions& o) {
    if (strcmp(key, "-m") == 0) o.model = val;
    else if (strcmp(key, "-w") == 0) o.wavPath = val;
    else if (strcmp(key, "-l") == 0) o.language = val;
    else if (strcmp(key, "--vad") == 0) o.vadModel = val;
    else if (strcmp(key, "--onnx") == 0) o.onnxDir = val;
    else if (strcmp(key, "-j") == 0) o.workers = std::min(64, std::max(1, atoi(val)));
    else if (strcmp(key, "-t") == 0) o.threads = std::min(8, std::max(1, atoi(val)));
    else if (strcmp(key, "-r") == 0) o.repeat = std::min(1000, std::max(1, atoi(val)));
    else return false;
    return true;
}

bool ParseBenchArgs(int argc, char** argv, BenchOptions& opts) {
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--no-dynamic-ctx") == 0) {
            opts.dynamicCtx = false;
            continue;
        }
        if (i + 1 >= argc || !ApplyArg(argv[i], argv[i + 1], opts)) {
            PrintUsage();
            return false;
        }
        ++i;
    }
    if ((opts.model.empty() && opts.onnxDir.empty()) || opts.wavPath.empty()) {
        PrintUsage();
        return false;
    }
    return true;
}

std::vector<std::string> ListWavFiles(const std::string& path) {
    namespace fs = std::filesystem;
    std::vector<std::string> out;
    std::error_code ec;
    if (!fs::is_directory(path, ec)) {
        out.push_back(path);
        return out;
    }
    for (const auto& e : fs::directory_iterator(path, ec)) {
        if (e.is_regular_file() && e.path().extension() == ".wav") out.push_back(e.path().string());
    }
    std::sort(out.begin(), out.end());
    return out;
}
