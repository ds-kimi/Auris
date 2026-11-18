-- Talkers: each bot plays a clip through realbots.VoiceAudio, then stays
-- quiet long enough that it speaks `duty` of the time, like a player.

AurisIngame = AurisIngame or {}

-- the stock client sends its end-of-voice signal shortly after the last
-- packet; 250 ms also lets the last Opus frames reach the hook
local FLUSH_DELAY = 0.25

---@param name string
---@return Player|nil
local function FindPlayer(name)
    for _, p in ipairs(player.GetAll()) do
        if p:Nick() == name then return p end
    end
end

-- mirrors sv_auris_feed: the net message a real client sends ends up in
-- auris.Flush(VoiceKey); realbots have no client Lua to send it
---@param scn table
---@param ply Player
---@param clip string
local function Flush(scn, ply, clip)
    if not IsValid(ply) then return end
    if scn.params.auris then
        AurisIngame.Expect(scn, ply:EntIndex(), clip)
        auris.Flush(Auris.VoiceKey(ply))
    end
end

---@param scn table
---@param i integer talker index
function AurisIngame.Talk(scn, i)
    if scn.stopped then return end
    local clips, id = scn.params.clips, scn.ids[i]
    local clip = clips[(scn.turn[i] % #clips) + 1]
    scn.turn[i] = scn.turn[i] + 1
    local ply = FindPlayer(scn.names[i])
    local seconds = realbots.VoiceAudio(id, clip, false) or 0
    scn.spoken = (scn.spoken or 0) + seconds
    timer.Simple(seconds + FLUSH_DELAY, function() Flush(scn, ply, clip) end)
    local idle = seconds / scn.params.duty - seconds
    timer.Simple(seconds + idle * (0.5 + math.random()), function() AurisIngame.Talk(scn, i) end)
end
