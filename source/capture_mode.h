// How faithfully captured audio has to match the clock it was spoken on.
#pragma once

// Transcription does not care where the pauses in an utterance were, so by
// default the capture path keeps only voiced audio: silence opcodes are skipped
// and gaps between packets are closed up. The result is shorter than the time it
// covers, which is free for whisper and wrong for anything that plays the audio
// back in step with what was happening while it was spoken.
//
// Consumers that want the recording rather than the words — replay, evidence
// capture, anything lining audio up against a timeline — turn this on and get
// real time back, at the cost of silence in the buffer.
void SetPreserveTimeline(bool preserve);
bool PreserveTimeline();
