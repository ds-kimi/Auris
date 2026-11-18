// Transcripts handed from the workers to the Lua poll loop
#pragma once
#include "auris_context.h"

void PushResult(TranscriptResult result);
bool PopResult(TranscriptResult& out);
