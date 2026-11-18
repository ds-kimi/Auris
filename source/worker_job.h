// Transcribes one clip on one worker's private whisper_state
#pragma once
#include "auris_context.h"
#include "whisper.h"

// ctx is shared read-only between workers; state belongs to the caller alone
void RunTranscriptionJob(whisper_context* ctx, whisper_state* state,
                         const TranscriptJob& job, bool acftModel);
