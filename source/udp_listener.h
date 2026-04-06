// UDP listener for receiving 8bit voice broadcast packets
#pragma once
#include <thread>
#include <atomic>
#include <cstdint>
#include <string>

void StartUDPListener(int port);
void StopUDPListener();
// Get steamid64 string for a buffer key
std::string GetSteamID64ForKey(int key);
