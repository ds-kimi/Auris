// Per-thread CPU facts: where it runs, what it burned, where it may run
#pragma once
#include <cstdint>

// Logical CPU of the calling thread, -1 when unknown or outside group 0
int CurrentCore();
// CPU time (user + kernel) consumed by the calling thread, in seconds
double ThreadCpuSeconds();
// Restricts the calling thread to mask; false when the OS refused it
bool SetCurrentThreadMask(uint64_t mask);
