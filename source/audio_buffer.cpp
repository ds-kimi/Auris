// Per-player audio accumulation buffer
#include "audio_buffer.h"
#include "capture_mode.h"

// Decoded audio is always 16 kHz mono by the time it reaches here.
static const int BUFFER_RATE = 16000;

// Packets do not arrive on a perfectly even cadence, and a buffer that is a few
// milliseconds behind the clock is normal rather than a gap. Only a shortfall
// larger than this is treated as real silence worth writing back in.
static const size_t JITTER_SAMPLES = BUFFER_RATE / 10;

static AudioBuffer g_buffer;

AudioBuffer& GetAudioBuffer() {
    return g_buffer;
}

// With timeline fidelity on, pads out to wall clock before appending so the
// buffer stays a recording of the microphone rather than a concatenation of
// whatever happened to be voiced. A talker who goes quiet simply stops sending
// packets, and no silence opcode marks that, so the clock is the only thing that
// knows the gap was there.
void AudioBuffer::Append(
    int userid, const float* data, int count
) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto& stream = m_buffers[userid];
    auto now = std::chrono::steady_clock::now();

    if (stream.pcm.empty()) {
        stream.opened = now;
    } else if (PreserveTimeline()) {
        double elapsed = std::chrono::duration<double>(now - stream.opened).count();
        size_t expected = (size_t)(elapsed * BUFFER_RATE);
        size_t projected = stream.pcm.size() + (size_t)count;

        if (expected > projected + JITTER_SAMPLES) {
            stream.pcm.insert(stream.pcm.end(), expected - projected, 0.0f);
        }
    }

    stream.pcm.insert(stream.pcm.end(), data, data + count);
}

std::vector<float> AudioBuffer::Flush(int userid) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_buffers.find(userid);
    if (it == m_buffers.end()) return {};

    std::vector<float> out = std::move(it->second.pcm);
    m_buffers.erase(it);
    return out;
}

std::vector<int> AudioBuffer::GetActiveKeys() {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<int> keys;
    for (auto& pair : m_buffers) {
        if (!pair.second.pcm.empty())
            keys.push_back(pair.first);
    }
    return keys;
}

void AudioBuffer::Clear(int userid) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_buffers.erase(userid);
}
