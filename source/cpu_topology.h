// Logical CPUs of this process and how they share last-level caches
#pragma once
#include <cstdint>
#include <vector>

struct CpuTopology {
    uint64_t allMask = 1;            // CPUs the process may run on (first 64)
    std::vector<uint64_t> l3Masks;   // one mask per distinct L3 cache (CCD)
};

// Queried once; srcds never sees more than 64 CPUs in one processor group
const CpuTopology& GetCpuTopology();
// "0-7,16-23" -> bit mask of the first 64 CPUs; 0 when nothing parses
uint64_t ParseCpuList(const char* list);
