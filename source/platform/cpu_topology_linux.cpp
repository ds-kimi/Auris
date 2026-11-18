#ifndef _WIN32
#include "platform/cpu_topology.h"
#include <cstdio>

// Linux affinity is per thread: sched_getaffinity(0) on the main thread came
// back as the single core Auris (or gm_antiesp) had just pinned it to, and
// every worker landed on the game's own core. Online CPUs are the process
// view; the kernel intersects a set mask with any cpuset restriction itself.
static uint64_t ReadCpuFile(const char* path) {
    char line[256];
    FILE* file = std::fopen(path, "r");
    if (!file) return 0;
    bool read = std::fgets(line, sizeof(line), file) != nullptr;
    std::fclose(file);
    return read ? ParseCpuList(line) : 0;
}

static uint64_t ReadL3Mask(int cpu) {
    char path[96];
    std::snprintf(path, sizeof(path),
                  "/sys/devices/system/cpu/cpu%d/cache/index3/shared_cpu_list", cpu);
    return ReadCpuFile(path);
}

// one L3 per distinct shared_cpu_list; CPUs without the file (VMs, WSL
// sometimes) simply leave the list empty and placement falls back to masks
static CpuTopology QueryTopology() {
    CpuTopology topo;
    uint64_t online = ReadCpuFile("/sys/devices/system/cpu/online");
    if (online) topo.allMask = online;
    uint64_t seen = 0;
    for (int cpu = 0; cpu < 64; cpu++) {
        if (!(topo.allMask >> cpu & 1) || (seen >> cpu & 1)) continue;
        uint64_t mask = ReadL3Mask(cpu) & topo.allMask;
        if (!mask) continue;
        seen |= mask;
        topo.l3Masks.push_back(mask);
    }
    return topo;
}

const CpuTopology& GetCpuTopology() {
    static const CpuTopology topo = QueryTopology();
    return topo;
}
#endif
