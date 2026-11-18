// sv_lan servers (and test bots) hand every client SteamID64 0, so the
// SteamID key collapsed all of them into one buffer that Lua could not even
// flush (AccountID() is -1 there). The engine's player slot is the only other
// identity in reach without vtable calls whose layout varies per branch.
#include "voice_key.h"

#include <cstring>

// CBaseClient m_nClientSlot / m_nEntityIndex, seen from the IClient
// subobject SV_BroadcastVoiceData receives: two pointers in on every
// platform (offsets from gm_antiesp's per-platform tables)
static const size_t kSlotOffset = 2 * sizeof(void*);
static const int kMaxEntity = 255;

int VoiceKeyFor(IClient* cl, uint64_t sid64) {
    if (sid64 != 0) return (int)(uint32_t)sid64;
    int32_t slot = 0, entity = 0;
    std::memcpy(&slot, (const char*)cl + kSlotOffset, sizeof(slot));
    std::memcpy(&entity, (const char*)cl + kSlotOffset + sizeof(slot), sizeof(entity));
    // the pair must look like the engine's own invariant, or the layout moved
    if (entity != slot + 1 || entity < 1 || entity > kMaxEntity) return 0;
    return -entity;
}

// Lua numbers are doubles: AccountID() above 2^31 overflowed the plain (int)
// cast (undefined behaviour) instead of wrapping like the hook's own key
int VoiceKeyFromLua(double value) {
    if (!(value >= -2147483648.0 && value <= 4294967295.0)) return 0;
    return (int)(uint32_t)(int64_t)value;
}
