#ifndef _WIN32
#include "cpu_topology.h"
#include <sched.h>
#include <cstdio>

static uint64_t ReadL3Mask(int cpu) {
    char path[96], line[256];
    std::snprintf(path, sizeof(path),
                  "/sys/devices/system/cpu/cpu%d/cache/index3/shared_cpu_list", cpu);
    FILE* file = std::fopen(path, "r");
    if (!file) return 0;
    bool read = std::fgets(line, sizeof(line), file) != nullptr;
    std::fclose(file);
    return read ? ParseCpuList(line) : 0;
}

// one L3 per distinct shared_cpu_list; CPUs without the file (VMs, WSL
// sometimes) simply leave the list empty and placement falls back to masks
static CpuTopology QueryTopology() {
    CpuTopology topo;
    cpu_set_t set;
    CPU_ZERO(&set);
    uint64_t all = 0;
    if (sched_getaffinity(0, sizeof(set), &set) == 0) {
        for (int cpu = 0; cpu < 64; cpu++)
            if (CPU_ISSET(cpu, &set)) all |= 1ull << cpu;
    }
    if (all) topo.allMask = all;
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
