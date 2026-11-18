// Vendored energy-based VAD from whisper.cpp/examples/common.cpp.
// Kept verbatim in logic; wrapped in auris:: to avoid ODR clash if upstream
// ever promotes these helpers into the public whisper.cpp API.
#define _USE_MATH_DEFINES
#include <cmath>
#include <cstdio>

#include "vad.h"

namespace auris {

static void high_pass_filter(std::vector<float>& data, float cutoff, float sample_rate) {
    const float rc    = 1.0f / (2.0f * (float)M_PI * cutoff);
    const float dt    = 1.0f / sample_rate;
    const float alpha = dt / (rc + dt);

    float y = data[0];
    for (size_t i = 1; i < data.size(); i++) {
        y = alpha * (y + data[i] - data[i - 1]);
        data[i] = y;
    }
}

bool vad_simple(std::vector<float>& pcmf32,
                int   sample_rate,
                int   last_ms,
                float vad_thold,
                float freq_thold,
                bool  verbose) {
    const int n_samples      = (int)pcmf32.size();
    const int n_samples_last = (sample_rate * last_ms) / 1000;

    // Chunks shorter than `last_ms` cannot be evaluated; treat as silence so
    // the caller skips them instead of decoding sub-second noise bursts.
    if (n_samples_last >= n_samples) {
        return false;
    }

    if (freq_thold > 0.0f) {
        high_pass_filter(pcmf32, freq_thold, (float)sample_rate);
    }

    float energy_all  = 0.0f;
    float energy_last = 0.0f;

    for (int i = 0; i < n_samples; i++) {
        energy_all += fabsf(pcmf32[i]);
        if (i >= n_samples - n_samples_last) {
            energy_last += fabsf(pcmf32[i]);
        }
    }

    energy_all  /= n_samples;
    energy_last /= n_samples_last;

    if (verbose) {
        fprintf(stderr,
                "auris::vad_simple: energy_all=%f energy_last=%f vad_thold=%f freq_thold=%f\n",
                energy_all, energy_last, vad_thold, freq_thold);
    }

    // Tail quieter than threshold * average => trailing silence => not speech.
    if (energy_last > vad_thold * energy_all) {
        return false;
    }
    return true;
}

} // namespace auris
