// Shared between the stream lifecycle and the frame decoder. Private to those
// two: everything else goes through opus_stream.h.
#pragma once
#include <opus.h>
#include <cstdint>

#include "resampler.h"

// Steam voice is opus at 24 kHz mono.
static const int OPUS_STREAM_RATE = 24000;
static const int OPUS_MAX_FRAME = 5760;

extern OpusDecoder *g_decoder;
extern PolyphaseResampler g_resampler;

// Frame sequence numbers are the sender's, not ours, so the first one seen
// after a reset is adopted rather than treated as a gap. Starting from zero
// made every utterance open with a burst of concealment for frames that were
// never missing, decoded through a decoder that had no history to conceal from.
extern uint16_t g_seq;
extern bool g_seqPrimed;
