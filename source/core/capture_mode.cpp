// Capture fidelity flag, read from the voice detour thread and written from Lua.
#include "capture_mode.h"

#include <atomic>

// Off by default: transcription is the common case and gains nothing from
// silence, so an operator who never asked for timeline fidelity does not pay
// for it in buffer size or inference time.
static std::atomic<bool> g_preserveTimeline( false );

void SetPreserveTimeline( bool preserve ) {
    g_preserveTimeline.store( preserve );
}

bool PreserveTimeline() {
    return g_preserveTimeline.load();
}
