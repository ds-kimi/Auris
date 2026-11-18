// Opus decode state for the steam voice stream, and the concealment that keeps
// a lossy stream the right length.
#pragma once
#include <vector>
#include <cstdint>

bool OpusStreamInit();
void OpusStreamShutdown();

// Drops decoder history. Anything decoded after this
// must not be filtered against audio from before the break.
void OpusStreamReset();

// Points the decode calls below at the stream of one speaker (voice buffer key),
// creating its decoder on first use. False when opus cannot allocate one.
bool OpusStreamSelect(int key);
void OpusSpeakersClear();

// Lets opus invent `count` missing frames and keeps what it produces. Opus is
// asked for concealment either way; throwing the result away leaves a hole
// where the audio should be, which is heard as a word going missing.
void OpusStreamConceal(int count, std::vector<float> &out);

// Decodes the frames inside one OP_CODEC_OPUSPLC chunk.
void OpusStreamDecodeFrames(const uint8_t *data, int len, std::vector<float> &out);
