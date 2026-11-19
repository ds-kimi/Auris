#pragma once
#include <vector>

void StoreCachedAudio(int key, const std::vector<float>& audio);
std::vector<float> TakeCachedAudio(int key);
