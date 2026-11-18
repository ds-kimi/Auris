// Minimal WAV loader for the benchmark: 16 kHz mono, PCM16 or float32
#pragma once
#include <string>
#include <vector>

// False when the file is unreadable or not 16 kHz mono PCM16/float32
bool ReadWav16kMono(const std::string& path, std::vector<float>& out);
