// Runtime configuration for whisper transcription
#pragma once
#include <string>
#include <mutex>

struct WhisperConfig {
    bool print_progress = false;
    bool print_timestamps = false;
    bool single_segment = true;
    bool no_context = true;
    std::string language = "en";
    int n_threads = 4;
};

WhisperConfig GetWhisperConfig();
void SetWhisperConfig(const WhisperConfig& cfg);
