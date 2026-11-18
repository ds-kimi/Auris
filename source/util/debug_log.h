#pragma once
#include <atomic>
#include <cstdio>

extern std::atomic<bool> g_debugEnabled;

// Debug print, only outputs when debug mode is on
#define WDEBUG(...) \
    do { if (g_debugEnabled) printf(__VA_ARGS__); } while(0)
