// Decodes steam voice packets (Opus) to PCM
#pragma once
#include <vector>
#include <cstdint>

bool InitSteamVoiceDecoder();
void ShutdownSteamVoiceDecoder();

// Decode a raw steam voice UDP packet to 16kHz float PCM
std::vector<float> DecodeSteamVoice(
    const uint8_t* data, int len
);
