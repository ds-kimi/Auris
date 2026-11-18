#include "result_queue.h"
#include <mutex>
#include <queue>

static std::mutex g_resultMutex;
static std::queue<TranscriptResult> g_results;

void PushResult(TranscriptResult result) {
    std::lock_guard<std::mutex> lock(g_resultMutex);
    g_results.push(std::move(result));
}

bool PopResult(TranscriptResult& out) {
    std::lock_guard<std::mutex> lock(g_resultMutex);
    if (g_results.empty()) return false;
    out = std::move(g_results.front());
    g_results.pop();
    return true;
}
