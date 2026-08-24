#pragma once
#include <vector>

// Steam voice arrives at 24 kHz and everything downstream wants 16 kHz:
// upsample by 2, filter, decimate by 3.
static constexpr int RS_L = 2;
static constexpr int RS_M = 3;
// Input taps per output sample. Enough of them that the filter can actually
// reach its stopband before the frequencies that fold back into speech.
static constexpr int RS_K = 32;
static constexpr int RS_NTAPS = RS_L * RS_K;

// Stateful because the stream arrives one opus frame at a time. Resampling each
// frame in isolation meant the filter ran off the end of the frame and clamped
// to the last sample it had, so every 20 ms boundary produced its own small
// burst of distortion — a buzz at the frame rate that made replayed speech
// sound robotic. Carrying the tail across calls makes the joins invisible.
struct PolyphaseResampler {
    void Reset();
    void Process(const short *in, int count, std::vector<float> &out);

private:
    std::vector<short> m_history;
    // Both are indices on the stream as a whole, not into any one frame, so the
    // filter phase carries across a join instead of restarting at every frame.
    long long m_base = 0;   // stream index of m_history[0]
    long long m_next = 0;   // next output sample to produce
};
