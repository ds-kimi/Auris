// Transcripts handed from the workers to the Lua poll loop
#pragma once
#include "core/auris_context.h"

void PushResult(TranscriptResult result);
bool PopResult(TranscriptResult& out);
