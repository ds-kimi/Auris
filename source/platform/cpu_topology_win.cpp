#ifdef _WIN32
#include "cpu_topology.h"
#include <windows.h>

// Masks are processor group 0 only: on a >64-CPU host another group reuses
// the same bit numbers for different cores
static void ReadL3Caches(CpuTopology& topo) {
    DWORD bytes = 0;
    GetLogicalProcessorInformationEx(RelationCache, nullptr, &bytes);
    if (bytes == 0) return;
    std::vector<uint8_t> buffer(bytes);
    auto* base = reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(buffer.data());
    if (!GetLogicalProcessorInformationEx(RelationCache, base, &bytes)) return;
    for (DWORD offset = 0; offset < bytes;) {
        auto* entry = reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(
            buffer.data() + offset);
        const CACHE_RELATIONSHIP& cache = entry->Cache;
        uint64_t mask = (uint64_t)cache.GroupMask.Mask & topo.allMask;
        if (cache.Level == 3 && cache.GroupMask.Group == 0 && mask) topo.l3Masks.push_back(mask);
        offset += entry->Size;
    }
}

static CpuTopology QueryTopology() {
    CpuTopology topo;
    DWORD_PTR process = 0, system = 0;
    GetProcessAffinityMask(GetCurrentProcess(), &process, &system);
    if (process) topo.allMask = (uint64_t)process;
    ReadL3Caches(topo);
    return topo;
}

const CpuTopology& GetCpuTopology() {
    static const CpuTopology topo = QueryTopology();
    return topo;
}
#endif
