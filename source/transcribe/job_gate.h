// Decides whether a clip is worth transcribing, before either backend runs
#pragma once
#include "core/auris_config.h"
#include "core/auris_context.h"
#include "transcribe/worker_job.h"

// false = skip the clip; job.audio may be cropped in place to the speech span
bool GateJobAudio(const WhisperConfig& cfg, const WorkerSlot& slot, TranscriptJob& job, float dur);
