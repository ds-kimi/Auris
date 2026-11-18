// Wall time would charge a worker for the time srcds preempted it; thread
// CPU time is what the server actually paid for a transcript.
#include "thread_cpu.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <sched.h>
#include <time.h>
#endif

int CurrentCore() {
#ifdef _WIN32
    PROCESSOR_NUMBER number;
    GetCurrentProcessorNumberEx(&number);
    return number.Group == 0 ? (int)number.Number : -1;
#else
    int cpu = sched_getcpu();
    return cpu >= 0 && cpu < 64 ? cpu : -1;
#endif
}

double ThreadCpuSeconds() {
#ifdef _WIN32
    FILETIME created, exited, kernel, user;
    if (!GetThreadTimes(GetCurrentThread(), &created, &exited, &kernel, &user)) return 0.0;
    ULARGE_INTEGER k, u;
    k.LowPart = kernel.dwLowDateTime; k.HighPart = kernel.dwHighDateTime;
    u.LowPart = user.dwLowDateTime;   u.HighPart = user.dwHighDateTime;
    return (double)(k.QuadPart + u.QuadPart) / 1e7;
#else
    timespec ts;
    if (clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts) != 0) return 0.0;
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
#endif
}

bool SetCurrentThreadMask(uint64_t mask) {
    if (!mask) return false;
#ifdef _WIN32
    // a 32-bit build only addresses the first 32 CPUs anyway
    return SetThreadAffinityMask(GetCurrentThread(), (DWORD_PTR)mask) != 0;
#else
    cpu_set_t set;
    CPU_ZERO(&set);
    for (int cpu = 0; cpu < 64; cpu++)
        if (mask >> cpu & 1) CPU_SET(cpu, &set);
    return sched_setaffinity(0, sizeof(set), &set) == 0;
#endif
}
