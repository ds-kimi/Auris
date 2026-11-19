// The module is built for AVX2+FMA+F16C; on an older CPU whisper.cpp dies with an
// illegal-instruction crash that takes srcds down, so the local backend refuses to start instead.
#include "platform/cpu_features.h"

#if defined(_MSC_VER)
#include <intrin.h>
#include <immintrin.h>
static void QueryCpuid(int leaf, int sub, unsigned int regs[4]) {
    int out[4];
    __cpuidex(out, leaf, sub);
    for (int i = 0; i < 4; i++) regs[i] = (unsigned int)out[i];
}
static unsigned long long ReadXcr0() { return _xgetbv(0); }
#else
#include <cpuid.h>
static void QueryCpuid(int leaf, int sub, unsigned int regs[4]) {
    __cpuid_count(leaf, sub, regs[0], regs[1], regs[2], regs[3]);
}
// inline asm so the file does not need -mxsave
static unsigned long long ReadXcr0() {
    unsigned int lo, hi;
    __asm__ volatile("xgetbv" : "=a"(lo), "=d"(hi) : "c"(0));
    return ((unsigned long long)hi << 32) | lo;
}
#endif

bool CpuHasRequiredFeatures() {
    unsigned int r[4] = {0, 0, 0, 0};
    QueryCpuid(0, 0, r);
    if (r[0] < 7) return false;

    QueryCpuid(1, 0, r);
    bool fma  = (r[2] & (1u << 12)) != 0;
    bool f16c = (r[2] & (1u << 29)) != 0;
    // CPU bits alone are not enough: a hypervisor or OS can leave YMM state disabled
    bool osxsave = (r[2] & (1u << 27)) != 0;
    bool avx     = (r[2] & (1u << 28)) != 0;
    if (!osxsave || !avx || (ReadXcr0() & 0x6) != 0x6) return false;

    QueryCpuid(7, 0, r);
    bool avx2 = (r[1] & (1u << 5)) != 0;
    return avx2 && fma && f16c;
}
