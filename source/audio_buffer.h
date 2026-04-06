// Per-player audio accumulation buffer
#pragma once
#include <vector>
#include <unordered_map>
#include <mutex>

class AudioBuffer {
public:
    void Append(int userid, const float* data, int count);
    std::vector<float> Flush(int userid);
    // Returns all buffered userids that have data
    std::vector<int> GetActiveKeys();
    void Clear(int userid);

private:
    std::mutex m_mutex;
    std::unordered_map<int, std::vector<float>> m_buffers;
};

AudioBuffer& GetAudioBuffer();
