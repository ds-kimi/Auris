// Lua bindings for the capture fidelity flag
#include "lua_bindings.h"
#include "capture_mode.h"

using namespace GarrysMod::Lua;

// auris.SetPreserveTimeline(bool) -> nil
// Keeps silence in the captured audio so a clip lines up with the clock it was
// spoken on. Set it before the utterance is spoken: padding happens as packets
// arrive, so flipping it mid-sentence only affects what comes after.
LUA_FUNCTION(Whisper_SetPreserveTimeline) {
    LUA->CheckType(1, Type::Bool);
    SetPreserveTimeline(LUA->GetBool(1));
    return 0;
}

// auris.PreservesTimeline() -> bool
LUA_FUNCTION(Whisper_PreservesTimeline) {
    LUA->PushBool(PreserveTimeline());
    return 1;
}
