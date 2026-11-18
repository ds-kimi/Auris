// Which CPUs the transcription workers may use, given the game's core
#pragma once
#include "platform/cpu_topology.h"
#include <cstdint>

// overrideMask (worker_cores) wins when it leaves any usable CPU; otherwise the
// largest L3 group without mainCore, else every CPU but mainCore, else all
uint64_t PickWorkerMask(const CpuTopology& topo, int mainCore, uint64_t overrideMask);
