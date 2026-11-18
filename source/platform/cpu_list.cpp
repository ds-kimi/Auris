// Same list syntax as sysfs and taskset, so worker_cores reads like the
// output of `lscpu` or the cache files the topology query parses
#include "platform/cpu_topology.h"
#include <cstdlib>

uint64_t ParseCpuList(const char* list) {
    uint64_t mask = 0;
    // stops at the trailing newline or anything else that is not a number
    for (const char* p = list; p && *p >= '0' && *p <= '9';) {
        char* end = nullptr;
        long first = std::strtol(p, &end, 10);
        long last = *end == '-' ? std::strtol(end + 1, &end, 10) : first;
        for (long cpu = first; cpu <= last && cpu < 64; cpu++) mask |= 1ull << cpu;
        p = *end == ',' ? end + 1 : end;
    }
    return mask;
}
