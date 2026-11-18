// Manages auris model loading and transcription
#pragma once
#include "whisper.h"
#include <string>
#include <mutex>
#include <vector>
#include <queue>
#include <thread>
#include <atomic>
#include <chrono>
#include <condition_variable>

struct TranscriptResult {
    int key;
    std::string text;
};

struct TranscriptJob {
    int key;
    std::vector<float> audio;
    // set by the queue; the auto_workers tuner reads how long the clip waited
    std::chrono::steady_clock::time_point queuedAt{};
};

bool InitWhisper(const std::string& modelPath);
void ShutdownWhisper();
void QueueTranscription(int key, std::vector<float> audio);
bool PollResult(TranscriptResult& out);
