#include "bench_stats.h"
#include <algorithm>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <psapi.h>
#else
#include <sys/resource.h>
#endif

double ProcessCpuSeconds() {
#ifdef _WIN32
    FILETIME c, e, k, u;
    GetProcessTimes(GetCurrentProcess(), &c, &e, &k, &u);
    auto toSec = [](FILETIME t) {
        return (((unsigned long long)t.dwHighDateTime << 32) | t.dwLowDateTime) / 1e7;
    };
    return toSec(k) + toSec(u);
#else
    rusage r;
    getrusage(RUSAGE_SELF, &r);
    return r.ru_utime.tv_sec + r.ru_stime.tv_sec + (r.ru_utime.tv_usec + r.ru_stime.tv_usec) / 1e6;
#endif
}

double PeakRssMiB() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
    return pmc.PeakWorkingSetSize / (1024.0 * 1024.0);
#else
    rusage r;
    getrusage(RUSAGE_SELF, &r);
    return r.ru_maxrss / 1024.0;  // Linux reports KiB
#endif
}

double Percentile(std::vector<double> v, double q) {
    if (v.empty()) return 0.0;
    std::sort(v.begin(), v.end());
    size_t i = (size_t)(q * (v.size() - 1) + 0.5);
    return v[std::min(i, v.size() - 1)];
}
