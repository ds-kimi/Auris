// Decodes steam voice packets to PCM for whisper
#include "steam_voice.h"
#include <opus.h>
#include "debug_log.h"
#include <cstring>

static const int STEAM_RATE = 24000;
static const int WHISPER_RATE = 16000;
static const int FRAME_SIZE = 480;
static const int MAX_FRAME = 5760;
static OpusDecoder* g_decoder = nullptr;
static uint16_t g_seq = 0;

enum {
    OP_SILENCE = 0,
    OP_CODEC_OPUSPLC = 6,
    OP_SAMPLERATE = 11
};

bool InitSteamVoiceDecoder() {
    int err = 0;
    g_decoder = opus_decoder_create(STEAM_RATE, 1, &err);
    if (err != OPUS_OK || !g_decoder) {
        WDEBUG("[Whisper] Opus init failed: %d\n", err);
        return false;
    }
    WDEBUG("[Whisper] Opus decoder initialized\n");
    return true;
}

void ShutdownSteamVoiceDecoder() {
    if (g_decoder) {
        opus_decoder_destroy(g_decoder);
        g_decoder = nullptr;
    }
}

static void Resample(
    const short* in, int inCount,
    std::vector<float>& out
) {
    double ratio = (double)WHISPER_RATE / STEAM_RATE;
    int outCount = (int)(inCount * ratio);
    size_t base = out.size();
    out.resize(base + outCount);

    for (int i = 0; i < outCount; i++) {
        double srcIdx = i / ratio;
        int i0 = (int)srcIdx;
        int i1 = i0 + 1;
        if (i1 >= inCount) i1 = inCount - 1;
        double frac = srcIdx - i0;
        double val = in[i0] * (1.0 - frac) + in[i1] * frac;
        out[base + i] = (float)(val / 32768.0);
    }
}

// Decode the opus frames inside OP_CODEC_OPUSPLC payload
// Format: [uint16 len][uint16 seq][len bytes]...
// len=0xFFFF means reset
static int DecodeOpusFrames(
    const uint8_t* data, int len, std::vector<float>& out
) {
    const uint8_t* p = data;
    const uint8_t* end = data + len;
    short pcm[MAX_FRAME];
    int total = 0;

    while (p + sizeof(uint16_t) <= end) {
        uint16_t frameLen = *(const uint16_t*)p;
        p += sizeof(uint16_t);

        if (frameLen == 0xFFFF) {
            opus_decoder_ctl(g_decoder, OPUS_RESET_STATE);
            g_seq = 0;
            continue;
        }

        if (p + sizeof(uint16_t) > end) break;
        uint16_t seq = *(const uint16_t*)p;
        p += sizeof(uint16_t);

        if (seq < g_seq) {
            opus_decoder_ctl(g_decoder, OPUS_RESET_STATE);
        } else if (seq > g_seq) {
            uint16_t lost = seq - g_seq;
            if (lost > 10) lost = 10;
            for (uint16_t i = 0; i < lost; i++) {
                opus_decode(
                    g_decoder, NULL, 0, pcm, MAX_FRAME, 0
                );
            }
        }
        g_seq = seq + 1;

        if (frameLen == 0 || p + frameLen > end) break;

        int samples = opus_decode(
            g_decoder, p, frameLen, pcm, MAX_FRAME, 0
        );
        p += frameLen;

        if (samples > 0) {
            Resample(pcm, samples, out);
            total += samples;
        }
    }
    return total;
}

std::vector<float> DecodeSteamVoice(
    const uint8_t* data, int len
) {
    std::vector<float> result;
    if (!g_decoder || len < 12) return result;

    // Packet: uint64 steamid + opcodes + uint32 CRC
    const uint8_t* p = data + sizeof(uint64_t);
    const uint8_t* end = data + len - sizeof(uint32_t);

    while (p < end) {
        if (p + sizeof(char) > end) break;
        uint8_t opcode = *p;
        p += sizeof(char);

        switch (opcode) {
        case OP_SILENCE: {
            if (p + sizeof(uint16_t) > end) return result;
            p += sizeof(uint16_t);
            break;
        }
        case OP_SAMPLERATE: {
            if (p + sizeof(uint16_t) > end) return result;
            p += sizeof(uint16_t);
            break;
        }
        case OP_CODEC_OPUSPLC: {
            if (p + sizeof(uint16_t) > end) return result;
            uint16_t frameDataLen = *(const uint16_t*)p;
            p += sizeof(uint16_t);
            if (p + frameDataLen > end) return result;

            DecodeOpusFrames(p, frameDataLen, result);
            p += frameDataLen;
            break;
        }
        default:
            WDEBUG("[Whisper] Unknown opcode %d\n", opcode);
            return result;
        }
    }
    return result;
}
