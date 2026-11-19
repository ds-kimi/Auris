// Keeps transcription workers off the core running the srcds tick
#pragma once
#include "core/auris_config.h"
#include <cstdint>

// Reads affinity / pin_main_thread / worker_cores; call on the main thread
// before the workers start, it also performs the first placement
void ConfigurePlacement(const WhisperConfig& cfg);
// Main thread, cheap: re-reads the game's core at most once per second and
// publishes a new worker mask when it moved
void RefreshPlacement();
// Worker thread: applies the latest mask when it changed since `seen`
void ApplyWorkerPlacement(uint32_t& seen);
