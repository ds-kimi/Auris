// Unlike gm_antiesp (whose workers want srcds' hot L3), whisper and ONNX
// stream megabytes of weights per clip: sharing the game's L3 would evict
// the tick's working set, so workers prefer another CCD when there is one.
#include "transcribe/worker_mask.h"

static int CountBits(uint64_t mask) {
    int n = 0;
    for (; mask; mask &= mask - 1) n++;
    return n;
}

static uint64_t LargestForeignL3(const CpuTopology& topo, uint64_t mainBit) {
    uint64_t best = 0;
    for (uint64_t mask : topo.l3Masks) {
        if (mask & mainBit) continue;
        if (CountBits(mask) > CountBits(best)) best = mask;
    }
    return best;
}

uint64_t PickWorkerMask(const CpuTopology& topo, int mainCore, uint64_t overrideMask) {
    uint64_t mainBit = mainCore >= 0 && mainCore < 64 ? 1ull << mainCore : 0;
    uint64_t custom = overrideMask & topo.allMask & ~mainBit;
    if (custom) return custom;
    uint64_t foreign = LargestForeignL3(topo, mainBit);
    if (foreign) return foreign;
    uint64_t rest = topo.allMask & ~mainBit;
    return rest ? rest : topo.allMask;
}
