// Busy workers cost L3 and memory bandwidth to the tick: start with one and
// only add workers while clips actually wait.
#include "transcribe/worker_tuner.h"
#include <atomic>
#include <chrono>
#include <mutex>

// ~1 s of waiting means captions already arrive late; under 0.1 s the extra
// worker sits idle. 5 s between changes lets the EMA settle first.
static const double kRaiseWait = 1.0;
static const double kLowerWait = 0.1;
static const double kEmaAlpha = 0.2;

static std::mutex g_tunerMutex;
static std::atomic<int> g_active{1};
static int g_max = 1;
static bool g_enabled = false;
static double g_ema = 0.0;
static std::chrono::steady_clock::time_point g_nextChange;

void TunerReset(int maxWorkers, bool enabled) {
    std::lock_guard<std::mutex> lock(g_tunerMutex);
    g_max = maxWorkers < 1 ? 1 : maxWorkers;
    g_enabled = enabled;
    g_ema = 0.0;
    g_nextChange = std::chrono::steady_clock::now();
    g_active = enabled ? 1 : g_max;
}

void TunerObserve(double waitSeconds) {
    std::lock_guard<std::mutex> lock(g_tunerMutex);
    if (!g_enabled) return;
    g_ema += kEmaAlpha * (waitSeconds - g_ema);
    auto now = std::chrono::steady_clock::now();
    if (now < g_nextChange) return;
    int active = g_active.load();
    if (g_ema > kRaiseWait && active < g_max) active++;
    else if (g_ema < kLowerWait && active > 1) active--;
    else return;
    g_active = active;
    g_nextChange = now + std::chrono::seconds(5);
}

bool TunerWorkerActive(int worker) {
    return worker < g_active.load();
}

int TunerActiveWorkers() {
    return g_active.load();
}
