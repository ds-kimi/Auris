// Read-only counters of the transcription pool, for auris.GetStats()
#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

struct WorkerStatsView {
    uint64_t jobs = 0;
    double cpuSeconds = 0.0;
    double audioSeconds = 0.0;
    int core = -1;  // last CPU the worker ran a job on
};

struct PoolStatsView {
    uint64_t dropped = 0;
    double queueSeconds = 0.0;
    int activeWorkers = 0;
    std::vector<WorkerStatsView> workers;
};

void StatsReset(int workers);
void StatsRecordJob(int worker, double cpuSeconds, size_t samples, int core);
PoolStatsView StatsSnapshot();
