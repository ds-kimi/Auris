// Process-level measurements for the benchmark
#pragma once
#include <vector>

// User + kernel CPU seconds of the whole process (all threads)
double ProcessCpuSeconds();
// Peak resident set size in MiB
double PeakRssMiB();
// Value at fraction q (0..1) of the sorted samples; 0 when empty
double Percentile(std::vector<double> v, double q);
