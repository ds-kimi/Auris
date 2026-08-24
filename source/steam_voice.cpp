// Decodes steam voice packets to PCM for auris
#include "steam_voice.h"
#include "opus_stream.h"
#include "debug_log.h"
#include "resampler.h"
#include "capture_mode.h"
#include <cstring>

enum {
    OP_SILENCE = 0,
    OP_CODEC_OPUSPLC = 6,
    OP_SAMPLERATE = 11
};

// Everything downstream assumes the stream is the 24 kHz opus steam voice uses,
// including the fixed 2/3 resampling ratio. A stream at any other rate would
// come out at the wrong speed, so say so rather than producing it silently.
static const uint16_t EXPECTED_RATE = 24000;

bool InitSteamVoiceDecoder() {
    return OpusStreamInit();
}

void ShutdownSteamVoiceDecoder() {
    OpusStreamShutdown();
}

std::vector<float> DecodeSteamVoice(
    const uint8_t* data, int len
) {
    static bool warnedRate = false;
    std::vector<float> result;
    if (len < 12) return result;

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
            // The payload is a silent sample count at the stream rate. Dropping
            // it costs transcription nothing and deletes every pause from the
            // timeline: a sentence with breaths in it becomes one shorter run of
            // glued words, so audio replayed against the clock it was spoken on
            // runs out long before the speaker stopped. Written back only for
            // callers that asked for the recording rather than the words.
            uint16_t silent = *(const uint16_t*)p;
            p += sizeof(uint16_t);

            if (PreserveTimeline()) {
                result.insert(result.end(), (size_t)silent * RS_L / RS_M, 0.0f);
            }
            break;
        }
        case OP_SAMPLERATE: {
            if (p + sizeof(uint16_t) > end) return result;
            uint16_t rate = *(const uint16_t*)p;
            p += sizeof(uint16_t);

            if (rate != EXPECTED_RATE && !warnedRate) {
                warnedRate = true;
                WDEBUG("[Auris] stream declares %d Hz, decoding as %d\n",
                    (int)rate, (int)EXPECTED_RATE);
            }
            break;
        }
        case OP_CODEC_OPUSPLC: {
            if (p + sizeof(uint16_t) > end) return result;
            uint16_t frameDataLen = *(const uint16_t*)p;
            p += sizeof(uint16_t);
            if (p + frameDataLen > end) return result;

            OpusStreamDecodeFrames(p, frameDataLen, result);
            p += frameDataLen;
            break;
        }
        default:
            WDEBUG("[Auris] Unknown opcode %d\n", opcode);
            return result;
        }
    }
    return result;
}
