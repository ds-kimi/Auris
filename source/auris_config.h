// Runtime configuration for auris transcription
#pragma once
#include <string>
#include <mutex>

struct WhisperConfig {
    // --- core ---
    bool print_progress   = false;
    bool print_timestamps = false;
    bool single_segment   = false;
    bool no_context       = false;
    std::string language  = "";
    int n_threads         = 4;

    // --- sampling ---
    bool use_beam_search  = false;
    int greedy_best_of    = 5;
    int beam_size         = 5;

    // --- output ---
    bool translate             = false;
    bool detect_language       = false;
    bool no_timestamps         = false;
    bool print_special         = false;
    bool print_realtime        = false;
    bool debug_mode            = false;
    bool tdrz_enable           = false;

    // --- token timestamps ---
    bool token_timestamps = false;
    float thold_pt        = 0.01f;
    float thold_ptsum     = 0.01f;
    int max_len           = 0;
    bool split_on_word    = false;
    int max_tokens        = 0;

    // --- filtering ---
    bool suppress_blank        = true;
    bool suppress_nst          = false;
    float no_speech_thold      = 0.6f;
    std::string suppress_regex = "";
    std::string initial_prompt = "";
    bool carry_initial_prompt  = false;
    float vad_thold            = 0.6f;
    float freq_thold           = 100.0f;

    // --- decoding ---
    float temperature     = 0.0f;
    float temperature_inc = 0.2f;
    float entropy_thold   = 2.4f;
    float logprob_thold   = -1.0f;
    float max_initial_ts  = 1.0f;
    float length_penalty  = -1.0f;

    // --- context ---
    int n_max_text_ctx = 16384;
    int offset_ms      = 0;
    int duration_ms    = 0;
    int audio_ctx      = 0;
    bool dynamic_audio_ctx = true;
};

WhisperConfig GetWhisperConfig();
void SetWhisperConfig(const WhisperConfig& cfg);
