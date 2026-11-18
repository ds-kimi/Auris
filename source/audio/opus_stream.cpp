// Opus decoder lifecycle and packet-loss concealment.
#include "audio/opus_stream.h"
#include "audio/opus_stream_state.h"
#include "util/debug_log.h"

bool OpusStreamInit() {
    // decoders are per speaker and created lazily: fail at init, not on the
    // first voice packet, if this libopus cannot build one at all
    int err = 0;
    OpusDecoder *probe = opus_decoder_create(OPUS_STREAM_RATE, 1, &err);
    if (err != OPUS_OK || !probe) {
        WDEBUG("[Auris] Opus init failed: %d\n", err);
        return false;
    }
    opus_decoder_destroy(probe);
    WDEBUG("[Auris] Opus decoder initialized\n");
    return true;
}

void OpusStreamShutdown() {
    OpusSpeakersClear();
}

void OpusStreamReset() {
    if (!g_speaker) return;
    opus_decoder_ctl(g_speaker->decoder, OPUS_RESET_STATE);
    g_speaker->seqPrimed = false;
    g_speaker->seq = 0;
}

void OpusStreamConceal(int count, std::vector<float> &out) {
    if (!g_speaker) return;

    float pcm[OPUS_MAX_FRAME];
    for (int i = 0; i < count; i++) {
        int samples = opus_decode_float(g_speaker->decoder, NULL, 0, pcm, OPUS_CONCEAL_FRAME, 0);
        if (samples <= 0) return;

        out.insert(out.end(), pcm, pcm + samples);
    }
}
