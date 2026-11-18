// Buffer key of a speaker: the low 32 bits of the SteamID64, or the negative
// entity index when the server has no SteamID for the client.
#pragma once
#include <cstdint>

class IClient;

// 0 = no usable identity, the packet is dropped
int VoiceKeyFor(IClient* cl, uint64_t sid64);

// auris.Flush / FlushRaw argument -> the same key the hook filed the audio under
int VoiceKeyFromLua(double value);
