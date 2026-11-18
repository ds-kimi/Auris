// Runtime check for the x86 features the module is compiled for
#pragma once

// True when the CPU supports AVX2, FMA and F16C (the /arch:AVX2 / -mavx2 -mfma -mf16c build target)
bool CpuHasRequiredFeatures();
