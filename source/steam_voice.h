// Decodes steam voice packets (Opus) to PCM
#pragma once
#include <vector>
#include <cstdint>

bool InitSteamVoiceDecoder();
void ShutdownSteamVoiceDecoder();

// Decode a raw steam voice UDP packet to 16kHz float PCM, through the opus
// stream of speaker `key` (the voice buffer key)
std::vector<float> DecodeSteamVoice(int key,
    const uint8_t* data, int len
);
