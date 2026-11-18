// Per-speaker opus decoders, created on a speaker's first packet. Only the
// voice hook (engine main thread) decodes, so the table needs no lock.
#include "opus_stream.h"
#include "opus_stream_state.h"
#include "debug_log.h"

#include <unordered_map>

OpusSpeakerState *g_speaker = nullptr;
static std::unordered_map<int, OpusSpeakerState> g_speakers;

// SteamID keys accumulate over a long uptime; a decoder is ~20 KB, and
// starting every stream fresh once in a while costs one resync per speaker
static const size_t MAX_SPEAKERS = 512;

void OpusSpeakersClear() {
    for (auto &entry : g_speakers) opus_decoder_destroy(entry.second.decoder);
    g_speakers.clear();
    g_speaker = nullptr;
}

bool OpusStreamSelect(int key) {
    auto it = g_speakers.find(key);
    if (it == g_speakers.end()) {
        if (g_speakers.size() >= MAX_SPEAKERS) OpusSpeakersClear();
        int err = 0;
        OpusDecoder *decoder = opus_decoder_create(OPUS_STREAM_RATE, 1, &err);
        if (err != OPUS_OK || !decoder) {
            WDEBUG("[Auris] Opus decoder create failed: %d\n", err);
            g_speaker = nullptr;
            return false;
        }
        it = g_speakers.emplace(key, OpusSpeakerState{}).first;
        it->second.decoder = decoder;
    }
    g_speaker = &it->second;
    return true;
}
