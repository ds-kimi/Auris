// wparams.vad is not used: whisper_full keeps its VAD context on the shared
// whisper_context, which workers must treat as read-only.
#include "vad/silero_gate.h"
#include <algorithm>
#include <cstdio>

// Margin kept around the speech span so word onsets and trailing consonants
// that Silero scores just below threshold are not clipped
static const int kCropPadSamples = 16000 / 10;

bool SileroModelAvailable(const std::string& path) {
    FILE* f = path.empty() ? nullptr : fopen(path.c_str(), "rb");
    if (!f) {
        printf("[Auris] Silero VAD model not found (%s), VAD gate disabled\n", path.c_str());
        return false;
    }
    fclose(f);
    return true;
}

whisper_vad_context* InitSileroGate(const std::string& path) {
    whisper_vad_context_params vp = whisper_vad_default_context_params();
    vp.n_threads = 1;
    vp.use_gpu = false;
    return whisper_vad_init_from_file_with_params(path.c_str(), vp);
}

bool SileroCropToSpeech(whisper_vad_context* vctx, std::vector<float>& audio) {
    whisper_vad_params params = whisper_vad_default_params();
    whisper_vad_segments* segs = whisper_vad_segments_from_samples(
        vctx, params, audio.data(), (int)audio.size());
    if (!segs) return true;  // VAD failure must not cost the transcript
    int n = whisper_vad_segments_n_segments(segs);
    // segment times are centiseconds
    long first = n > 0 ? (long)(whisper_vad_segments_get_segment_t0(segs, 0) * 160.0f) : 0;
    long last  = n > 0 ? (long)(whisper_vad_segments_get_segment_t1(segs, n - 1) * 160.0f) : 0;
    whisper_vad_free_segments(segs);
    if (n <= 0) return false;

    long size  = (long)audio.size();
    long begin = std::max(0L, first - kCropPadSamples);
    long end   = std::min(size, last + kCropPadSamples);
    if (end <= begin) return false;
    audio.erase(audio.begin() + end, audio.end());
    audio.erase(audio.begin(), audio.begin() + begin);
    return true;
}
