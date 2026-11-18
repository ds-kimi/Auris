// Silero VAD gate: drops clips with no speech, crops the rest to the spoken span
#pragma once
#include "whisper.h"
#include <string>
#include <vector>

// Logs once and returns false when the path is empty or the file is missing
bool SileroModelAvailable(const std::string& path);
// One context per worker: whisper_vad_context keeps per-call LSTM state
whisper_vad_context* InitSileroGate(const std::string& path);
// False = no speech (skip the clip); true = audio cropped in place to the speech span
bool SileroCropToSpeech(whisper_vad_context* vctx, std::vector<float>& audio);
