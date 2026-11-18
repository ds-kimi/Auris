// Transcribes one clip on one worker's private whisper_state
#pragma once
#include "auris_context.h"
#include "whisper.h"

// Everything one worker mutates; ctx is shared read-only between workers
struct WorkerSlot {
    whisper_state* state = nullptr;
    whisper_vad_context* vad = nullptr;  // null = Silero gate disabled
};

// job.audio may be cropped in place to the detected speech span
void RunTranscriptionJob(whisper_context* ctx, const WorkerSlot& slot,
                         TranscriptJob& job, bool acftModel);
