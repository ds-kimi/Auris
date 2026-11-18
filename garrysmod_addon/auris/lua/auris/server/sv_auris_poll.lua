-- Single timer that drains the auris result queue and dispatches to subscribers.
-- All transcription delivery happens here; submodules never call auris.Poll().

-- sv_lan clients all share SteamID64 "0", so the SteamID cannot name the
-- speaker there; the C++ key (negative entity index) still does.
---@param sid string
---@param key integer|nil voice buffer key, nil from an older module binary
---@return Player|nil
function Auris.PlayerForResult(sid, key)
    if sid == "0" and isnumber(key) and key < 0 then
        local ply = Entity(-key)
        if IsValid(ply) and ply:IsPlayer() then return ply end
        return nil
    end
    return Auris.GetPlayerBySID64(sid)
end

-- ply is intentionally nil when the player disconnected before the result arrived.
-- Submodules that need a live player guard with `if not ply then return end`.
---@param sid string
---@param text string
---@param audio string|nil raw 16kHz float32 PCM binary, nil on cache miss
---@param key integer|nil
local function dispatchResult(sid, text, audio, key)
    local ply = Auris.PlayerForResult(sid, key)
    hook.Run("Auris_Transcription", ply, sid, text, audio)
end

-- Drain the entire queue in one tick so bursts from multiple simultaneous
-- speakers are not artificially spread across timer intervals.
local function drainQueue()
    if not Auris.IsReady() then return end
    local sid, text, audio, key = auris.Poll()
    while sid do
        dispatchResult(sid, text, audio, key)
        sid, text, audio, key = auris.Poll()
    end
end

function Auris.StartPollLoop()
    timer.Create("Auris_Poll", 0.1, 0, drainQueue)
end
