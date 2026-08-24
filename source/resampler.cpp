// Polyphase 24 kHz -> 16 kHz resampler for decoded steam voice.
#define _USE_MATH_DEFINES
#include <cmath>

#include "resampler.h"

#include <array>

// Anti-alias cutoff, as a divisor of the 48 kHz intermediate rate: 48000 / 2 /
// 3.2 = 7500 Hz, just under the 8 kHz Nyquist of the 16 kHz output.
//
// Dividing by RS_L instead put the cutoff at 12 kHz, which is correct for the
// interpolation stage and useless for the decimation that follows it: every
// frequency between 8 and 12 kHz passed at full level and folded back into the
// output, so 10 kHz of microphone hiss landed on top of speech at 6 kHz. That
// is what the metallic edge on replayed voice was.
static const double RS_CUTOFF_DIV = 3.2;
static const double RS_BETA = 8.0;

static double RS_I0(double x) {
    double s = 1.0, t = 1.0, hx = x * 0.5;
    for (int k = 1; k <= 25; k++) {
        double r = hx / k; t *= r * r; s += t;
        if (t < s * 1e-12) break;
    }
    return s;
}

// Windowed sinc, split into per-phase branches and normalised so each branch
// has unity DC gain, which keeps loudness constant across the phase cycle.
static std::array<float, RS_NTAPS> RS_BuildCoeffs() {
    std::array<float, RS_NTAPS> h{};
    double centre = (RS_NTAPS - 1) * 0.5;

    for (int i = 0; i < RS_NTAPS; i++) {
        double t = (i - centre) / RS_CUTOFF_DIV;
        double sinc = (std::abs(t) < 1e-9) ? 1.0 : std::sin(M_PI * t) / (M_PI * t);
        double r = 2.0 * i / (RS_NTAPS - 1) - 1.0;
        double kaiser = RS_I0(RS_BETA * std::sqrt(1.0 - r * r)) / RS_I0(RS_BETA);
        h[i] = (float)(sinc * kaiser);
    }

    for (int p = 0; p < RS_L; p++) {
        float sum = 0.f;
        for (int k = 0; k < RS_K; k++) sum += h[p + k * RS_L];
        if (sum > 0.f)
            for (int k = 0; k < RS_K; k++) h[p + k * RS_L] /= sum;
    }
    return h;
}

// Priming the history with silence costs a millisecond of fade-in on the first
// utterance and removes the special case for the very first frame.
void PolyphaseResampler::Reset() {
    m_history.assign(RS_K, 0);
    m_base = -RS_K;
    m_next = 0;
}

void PolyphaseResampler::Process(const short *in, int count, std::vector<float> &out) {
    static const std::array<float, RS_NTAPS> coeffs = RS_BuildCoeffs();

    if (count <= 0) return;
    if (m_history.empty()) Reset();

    std::vector<short> buf(m_history);
    buf.insert(buf.end(), in, in + count);
    const long long last = m_base + (long long)buf.size() - 1;

    // Half the taps sit ahead of the sample an output is centred on, so an
    // output waits until the input it needs has actually arrived rather than
    // being built out of a repeated edge sample.
    for (;; ++m_next) {
        long long n = (m_next * RS_M) / RS_L;
        if (n + RS_K / 2 - 1 > last) break;

        int phase = (int)((m_next * RS_M) % RS_L);
        float acc = 0.f;

        // The taps run backwards over the input while the coefficients run
        // forwards: y[i] = sum h[phase + k*L] * in[n - k]. Walking both forwards
        // reads the branch in time-reversed order, which for L = 2 is the other
        // branch entirely — half a sample of timing error applied to every
        // second output. That mirrors the signal around 8 kHz: a 2 kHz tone came
        // out with a 6 kHz twin only 11 dB down. Inharmonic, tracks pitch, and
        // sounds exactly like a ring modulator, which is the robot in the voice.
        for (int k = 0; k < RS_K; k++) {
            long long s = n + RS_K / 2 - 1 - k;
            if (s < m_base) s = m_base;
            acc += coeffs[phase + k * RS_L] * (float)buf[(size_t)(s - m_base)];
        }

        out.push_back(acc / 32768.f);
    }

    size_t keep = buf.size() < (size_t)RS_K ? buf.size() : (size_t)RS_K;
    m_history.assign(buf.end() - keep, buf.end());
    m_base += (long long)buf.size() - (long long)keep;
}
