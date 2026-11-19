// Builds whisper_full_params for one transcription job
#pragma once
#include "whisper.h"
#include "core/auris_config.h"

// acftModel: model fine-tuned for reduced audio_ctx (FUTO whisper-acft)
whisper_full_params BuildWhisperParams(const WhisperConfig& cfg, int nSamples, bool acftModel);
