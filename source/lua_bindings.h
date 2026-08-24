// Lua function bindings for the auris module
#pragma once
#include "GarrysMod/Lua/Interface.h"

extern int Whisper_Init(lua_State* L);
extern int Whisper_Flush(lua_State* L);
extern int Whisper_FlushAll(lua_State* L);
extern int Whisper_FlushRaw(lua_State* L);
extern int Whisper_Poll(lua_State* L);
extern int Whisper_Shutdown(lua_State* L);
extern int Whisper_Debug(lua_State* L);
extern int Whisper_IsDebug(lua_State* L);
extern int Whisper_SetConfig(lua_State* L);
extern int Whisper_GetConfig(lua_State* L);
extern int Whisper_SetPreserveTimeline(lua_State* L);
extern int Whisper_PreservesTimeline(lua_State* L);
