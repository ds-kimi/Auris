// Frame loop for one OP_CODEC_OPUSPLC chunk.
#include "opus_stream.h"
#include "opus_stream_state.h"
#include "debug_log.h"

// Concealing more than this is not loss recovery any more, it is inventing
// speech: a jump that large means a new talk session or a reordered stream, and
// the sequence is adopted instead.
static const int MAX_CONCEAL_FRAMES = 5;

// Brings the decoder into step with the sender's frame counter, concealing a
// short gap and resyncing on anything larger. The comparison is done on the
// signed 16-bit difference so the counter wrapping past 65535 reads as one step
// forward rather than as the stream running backwards.
static void SyncSequence(uint16_t seq, std::vector<float> &out) {
    if (!g_seqPrimed) {
        g_seqPrimed = true;
        g_seq = seq;
    }

    int delta = (int)(int16_t)(seq - g_seq);

    if (delta < 0) {
        // The sender restarted its counter, so nothing decoded before this
        // frame has any relation to what comes after it.
        OpusStreamReset();
        g_seqPrimed = true;
    } else if (delta > 0 && delta <= MAX_CONCEAL_FRAMES) {
        OpusStreamConceal(delta, out);
    }
    // A larger forward jump is picked up where the stream now is: the decoder
    // keeps its history, since resetting on ordinary packet loss would add an
    // artifact of its own on top of the gap.

    g_seq = seq + 1;
}

void OpusStreamDecodeFrames(const uint8_t *data, int len, std::vector<float> &out) {
    if (!g_decoder) return;

    const uint8_t *p = data;
    const uint8_t *end = data + len;
    short pcm[OPUS_MAX_FRAME];

    while (p + sizeof(uint16_t) <= end) {
        uint16_t frameLen = *(const uint16_t *)p;
        p += sizeof(uint16_t);

        if (frameLen == 0xFFFF) {
            OpusStreamReset();
            continue;
        }

        if (p + sizeof(uint16_t) > end) break;
        uint16_t seq = *(const uint16_t *)p;
        p += sizeof(uint16_t);

        SyncSequence(seq, out);

        if (frameLen == 0 || p + frameLen > end) break;

        int samples = opus_decode(g_decoder, p, frameLen, pcm, OPUS_MAX_FRAME, 0);
        p += frameLen;

        if (samples > 0) g_resampler.Process(pcm, samples, out);
    }

    // Bytes left over mean the chunk was not laid out the way this loop reads
    // it, and every frame after the misread is lost. Worth knowing about.
    if (p != end) {
        WDEBUG("[Auris] opus chunk parse left %d of %d bytes\n", (int)(end - p), len);
    }
}
