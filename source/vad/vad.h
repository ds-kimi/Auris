// Energy-based voice activity detection.
// Vendored from vendor/whisper.cpp/examples/common.{h,cpp} to avoid
// pulling the full example TU (SDL, sampling, similarity) into the module.
#pragma once
#include <vector>

namespace auris {

bool vad_simple(std::vector<float>& pcmf32,
                int   sample_rate,
                int   last_ms,
                float vad_thold,
                float freq_thold,
                bool  verbose);

} // namespace auris
