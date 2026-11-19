// Per-player audio accumulation buffer
#pragma once
#include <vector>
#include <unordered_map>
#include <mutex>
#include <chrono>

class AudioBuffer {
public:
    void Append(int userid, const float* data, int count);
    std::vector<float> Flush(int userid);
    // Returns all buffered userids that have data
    std::vector<int> GetActiveKeys();
    void Clear(int userid);

private:
    // A talker who goes quiet simply stops sending packets, so the audio alone
    // cannot say how long the gap was. The arrival time of the first packet is
    // kept so silence can be measured against the clock and written back in.
    struct Stream {
        std::vector<float> pcm;
        std::chrono::steady_clock::time_point opened;
    };

    std::mutex m_mutex;
    std::unordered_map<int, Stream> m_buffers;
};

AudioBuffer& GetAudioBuffer();
