// Decodes steam voice packets to PCM for auris
#include "audio/steam_voice.h"
#include "audio/opus_stream.h"
#include "util/debug_log.h"
#include "core/capture_mode.h"
#include <cstring>

enum {
    OP_SILENCE = 0,
    OP_CODEC_OPUSPLC = 6,
    OP_SAMPLERATE = 11
};

// Steam voice is encoded at 24 kHz. Opus decodes it at 16 kHz regardless, but
// silence counts arrive at the encoder rate and are scaled by this ratio, so a
// stream at any other rate would pad pauses wrongly: say so.
static const uint16_t EXPECTED_RATE = 24000;
static const int OUTPUT_RATE = 16000;

bool InitSteamVoiceDecoder() {
    return OpusStreamInit();
}

void ShutdownSteamVoiceDecoder() {
    OpusStreamShutdown();
}

// One opcode and its payload; returns the next read position, or nullptr
// when the packet is truncated or carries an opcode this decoder does not know.
static const uint8_t* DecodeOpcode(uint8_t opcode, const uint8_t* p, const uint8_t* end,
                                   std::vector<float>& result) {
    static bool warnedRate = false;
    if (p + sizeof(uint16_t) > end) return nullptr;
    uint16_t value = *(const uint16_t*)p;
    p += sizeof(uint16_t);
    switch (opcode) {
    case OP_SILENCE:
        // The payload is a silent sample count at the stream rate. Dropping
        // it costs transcription nothing and deletes every pause from the
        // timeline; written back only for callers that asked for the
        // recording rather than the words.
        if (PreserveTimeline()) result.insert(result.end(), (size_t)value * OUTPUT_RATE / EXPECTED_RATE, 0.0f);
        return p;
    case OP_SAMPLERATE:
        if (value != EXPECTED_RATE && !warnedRate) {
            warnedRate = true;
            WDEBUG("[Auris] stream declares %d Hz, decoding as %d\n", (int)value, (int)EXPECTED_RATE);
        }
        return p;
    case OP_CODEC_OPUSPLC:
        if (p + value > end) return nullptr;
        OpusStreamDecodeFrames(p, value, result);
        return p + value;
    default:
        WDEBUG("[Auris] Unknown opcode %d\n", opcode);
        return nullptr;
    }
}

std::vector<float> DecodeSteamVoice(int key, const uint8_t* data, int len) {
    std::vector<float> result;
    if (len < 12 || !OpusStreamSelect(key)) return result;

    // Packet: uint64 steamid + opcodes + uint32 CRC
    const uint8_t* p = data + sizeof(uint64_t);
    const uint8_t* end = data + len - sizeof(uint32_t);
    while (p && p < end) {
        uint8_t opcode = *p;
        p = DecodeOpcode(opcode, p + 1, end, result);
    }
    return result;
}
