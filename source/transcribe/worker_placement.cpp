// The OS may migrate the srcds main thread at any time; pinning it (as
// gm_antiesp does) keeps the mask computed for its core valid in between.
#include "transcribe/worker_placement.h"
#include "platform/cpu_topology.h"
#include "platform/thread_cpu.h"
#include "transcribe/worker_mask.h"
#include <atomic>
#include <chrono>
#include <cstdio>

static bool g_enabled = false;
static bool g_pinMain = false;
static uint64_t g_override = 0;
static int g_mainCore = -2;
static std::chrono::steady_clock::time_point g_nextCheck;
static std::atomic<uint64_t> g_workerMask{0};
static std::atomic<uint32_t> g_generation{0};

void ConfigurePlacement(const WhisperConfig& cfg) {
    g_enabled = cfg.affinity;
    g_pinMain = cfg.pin_main_thread;
    g_override = ParseCpuList(cfg.worker_cores.c_str());
    g_mainCore = -2;
    g_nextCheck = std::chrono::steady_clock::time_point();
    // a pool restarted with affinity off must not inherit the previous mask
    g_workerMask.store(0);
    RefreshPlacement();
}

void RefreshPlacement() {
    if (!g_enabled) return;
    auto now = std::chrono::steady_clock::now();
    if (now < g_nextCheck) return;
    g_nextCheck = now + std::chrono::seconds(1);
    int core = CurrentCore();
    if (core < 0 || core == g_mainCore) return;
    g_mainCore = core;
    uint64_t mask = PickWorkerMask(GetCpuTopology(), core, g_override);
    if (g_pinMain) SetCurrentThreadMask(1ull << core);
    g_workerMask.store(mask);
    g_generation.fetch_add(1);
    printf("[Auris] Placement: main thread on core %d, worker mask 0x%llx\n",
           core, (unsigned long long)mask);
}

void ApplyWorkerPlacement(uint32_t& seen) {
    uint32_t gen = g_generation.load();
    if (gen == seen) return;
    seen = gen;
    uint64_t mask = g_workerMask.load();
    if (mask) SetCurrentThreadMask(mask);
}
