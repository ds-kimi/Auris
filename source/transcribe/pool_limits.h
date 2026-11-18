// Sizing of the transcription worker pool and its backlog
#pragma once
#include "core/auris_config.h"
#include <cstddef>
#include <string>

// cfg.workers, or min(4, max(1, cores / 2)) when 0; always 1..8
int ResolveWorkerCount(const WhisperConfig& cfg);
// Queued audio (in samples) above which the oldest jobs are dropped
size_t MaxQueueSamples(const WhisperConfig& cfg);
// Model fine-tuned for reduced audio_ctx (FUTO whisper-acft), judged by filename
bool IsAcftModel(const std::string& modelPath);
