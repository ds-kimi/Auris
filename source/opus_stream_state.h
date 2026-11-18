// Shared between the stream lifecycle and the frame decoder. Private to those
// two: everything else goes through opus_stream.h.
#pragma once
#include <opus.h>
#include <cstdint>

// Steam voice is encoded at 24 kHz, but opus can decode any stream at any of
// its rates. Asking for 16 kHz, the rate whisper and the Lua API want, lets the
// codec band-limit internally and removes a resampling pass per frame.
static const int OPUS_STREAM_RATE = 16000;
// Longest opus frame (120 ms) at the decode rate.
static const int OPUS_MAX_FRAME = OPUS_STREAM_RATE * 120 / 1000;

extern OpusDecoder *g_decoder;

// Frame sequence numbers are the sender's, not ours, so the first one seen
// after a reset is adopted rather than treated as a gap. Starting from zero
// made every utterance open with a burst of concealment for frames that were
// never missing, decoded through a decoder that had no history to conceal from.
extern uint16_t g_seq;
extern bool g_seqPrimed;
