// Auto-detection costs an extra encoder pass and misfires on short noisy clips,
// and an unbounded decoder can loop for hundreds of tokens on a hallucination.
#include "job_limits.h"
#include <algorithm>

const char* ResolveJobLanguage(const WhisperConfig& cfg) {
    if (!cfg.language.empty()) return cfg.language.c_str();
    return cfg.detect_language ? nullptr : "en";
}

int ChooseMaxTokens(const WhisperConfig& cfg, int nSamples) {
    // ~6 tokens per second covers fast speech; +8 for punctuation and short clips
    int byLength = 8 + (6 * nSamples + 15999) / 16000;
    int cap = cfg.max_tokens > 0 ? cfg.max_tokens : 96;
    return std::min(cap, byLength);
}
