// Per-job decode bounds that keep one bad clip from stalling the worker
#pragma once
#include "auris_config.h"

// Language passed to whisper; never auto-detect unless detect_language is set
const char* ResolveJobLanguage(const WhisperConfig& cfg);
// Token budget scaled to clip length, capped by cfg.max_tokens (0 = cap 96)
int ChooseMaxTokens(const WhisperConfig& cfg, int nSamples);
