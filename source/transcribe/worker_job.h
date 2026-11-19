// Transcribes one clip on one worker's private whisper_state
#pragma once
#include "core/auris_context.h"
#include "whisper.h"

// Everything one worker mutates; ctx is shared read-only between workers
struct WorkerSlot {
    whisper_state* state = nullptr;      // null on the ONNX backend
    whisper_vad_context* vad = nullptr;  // null = Silero gate disabled
    int index = 0;                       // stats slot and tuner rank
};

// ctx null = ONNX backend; job.audio may be cropped in place to the speech span
void RunTranscriptionJob(whisper_context* ctx, const WorkerSlot& slot,
                         TranscriptJob& job, bool acftModel);
