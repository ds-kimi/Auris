// Manages whisper model loading and transcription
#pragma once
#include "whisper.h"
#include <string>
#include <mutex>
#include <vector>
#include <queue>
#include <thread>
#include <atomic>
#include <condition_variable>

struct TranscriptResult {
    int key;
    std::string text;
};

struct TranscriptJob {
    int key;
    std::vector<float> audio;
};

bool InitWhisper(const std::string& modelPath);
void ShutdownWhisper();
void QueueTranscription(int key, std::vector<float> audio);
bool PollResult(TranscriptResult& out);
