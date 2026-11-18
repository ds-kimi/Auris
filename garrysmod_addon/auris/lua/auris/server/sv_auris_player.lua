-- Player lookup and per-player rate limiting for flush signals.
-- Centralised here so sv_auris_feed and sv_auris_poll stay free of player logic.

-- One flush signal per player per this many seconds.
-- Below 0.5 s a modified client could spam auris.Flush on every frame.
local FLUSH_INTERVAL = 0.5

---@type table<string, number>  SteamID64 → earliest allowed next flush time
local _flushTimes = {}

-- Returns nil instead of an invalid entity so callers skip IsValid boilerplate.
---@param sid string
---@return Player|nil
function Auris.GetPlayerBySID64(sid)
    for _, ply in ipairs(player.GetAll()) do
        if ply:SteamID64() == sid then return ply end
    end
    return nil
end

---@param ply Player
---@return boolean  true = allowed, false = rate limited
function Auris.CheckRateLimit(ply)
    local sid = ply:SteamID64()
    if (_flushTimes[sid] or 0) > CurTime() then return false end
    _flushTimes[sid] = CurTime() + FLUSH_INTERVAL
    return true
end

-- Cleans up the rate-limit entry so long-running servers do not accumulate
-- stale SteamID64 keys indefinitely.
hook.Add("PlayerDisconnected", "Auris_CleanRateLimit", function(ply)
    _flushTimes[ply:SteamID64()] = nil
end)

-- Voice buffer key the C++ hook files this player's audio under: AccountID,
-- or the negative entity index on sv_lan servers, where every client has
-- SteamID64 "0" and AccountID() is -1.
---@param ply Player
---@return integer
function Auris.VoiceKey(ply)
    local account = ply:AccountID()
    if isnumber(account) and account > 0 then return account end
    return -ply:EntIndex()
end
