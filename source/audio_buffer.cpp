// Per-player audio accumulation buffer
#include "audio_buffer.h"

static AudioBuffer g_buffer;

AudioBuffer& GetAudioBuffer() {
    return g_buffer;
}

void AudioBuffer::Append(
    int userid, const float* data, int count
) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto& buf = m_buffers[userid];
    buf.insert(buf.end(), data, data + count);
}

std::vector<float> AudioBuffer::Flush(int userid) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_buffers.find(userid);
    if (it == m_buffers.end()) return {};

    std::vector<float> out = std::move(it->second);
    m_buffers.erase(it);
    return out;
}

std::vector<int> AudioBuffer::GetActiveKeys() {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<int> keys;
    for (auto& pair : m_buffers) {
        if (!pair.second.empty())
            keys.push_back(pair.first);
    }
    return keys;
}

void AudioBuffer::Clear(int userid) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_buffers.erase(userid);
}
