// Opus decoder lifecycle and packet-loss concealment.
#include "opus_stream.h"
#include "opus_stream_state.h"
#include "debug_log.h"

OpusDecoder *g_decoder = nullptr;
PolyphaseResampler g_resampler;
uint16_t g_seq = 0;
bool g_seqPrimed = false;

bool OpusStreamInit() {
    int err = 0;
    g_decoder = opus_decoder_create(OPUS_STREAM_RATE, 1, &err);
    if (err != OPUS_OK || !g_decoder) {
        WDEBUG("[Auris] Opus init failed: %d\n", err);
        return false;
    }

    OpusStreamReset();
    WDEBUG("[Auris] Opus decoder initialized\n");
    return true;
}

void OpusStreamShutdown() {
    if (g_decoder) {
        opus_decoder_destroy(g_decoder);
        g_decoder = nullptr;
    }
    g_seqPrimed = false;
}

void OpusStreamReset() {
    if (g_decoder) opus_decoder_ctl(g_decoder, OPUS_RESET_STATE);
    g_resampler.Reset();
    g_seqPrimed = false;
    g_seq = 0;
}

void OpusStreamConceal(int count, std::vector<float> &out) {
    if (!g_decoder) return;

    short pcm[OPUS_MAX_FRAME];
    for (int i = 0; i < count; i++) {
        int samples = opus_decode(g_decoder, NULL, 0, pcm, OPUS_MAX_FRAME, 0);
        if (samples <= 0) return;

        g_resampler.Process(pcm, samples, out);
    }
}
