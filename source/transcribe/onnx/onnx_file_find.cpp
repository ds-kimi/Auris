// Zipformer exports carry the training epoch in the file name
// (encoder-epoch-99-avg-1.int8.onnx), so a fixed name would force operators
// to rename files; int8 wins because fp32 doubles the CPU cost for no gain
#include "transcribe/onnx/onnx_file_find.h"
#include <algorithm>
#include <filesystem>
#include <vector>

static bool EndsWith(const std::string& s, const std::string& tail) {
    return s.size() >= tail.size() && s.compare(s.size() - tail.size(), tail.size(), tail) == 0;
}

std::string FindOnnxFile(const std::string& dir, const std::string& prefix) {
    namespace fs = std::filesystem;
    std::vector<std::string> names;
    std::error_code ec;
    for (const auto& e : fs::directory_iterator(dir, ec)) {
        std::string n = e.path().filename().string();
        if (e.is_regular_file(ec) && n.rfind(prefix, 0) == 0 && EndsWith(n, ".onnx")) names.push_back(n);
    }
    std::sort(names.begin(), names.end());
    for (const std::string& n : names) {
        if (EndsWith(n, ".int8.onnx")) return dir + "/" + n;
    }
    return names.empty() ? std::string() : dir + "/" + names.front();
}
