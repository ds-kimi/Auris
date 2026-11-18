-- auris_load: N realbots talk real speech; records Auris latency per clip,
-- tick gaps, process CPU and RSS over the measured window.

AurisIngame = AurisIngame or {}

-- mean clip of the corpus: starts spread over one talk period, otherwise
-- every talker opens with a clip in the same few seconds and the first
-- burst (128 clips at once) is what the queue measures, not steady chat
local MEAN_CLIP = 4
-- the last clips are still in the workers when talking stops
local TAIL = 20

---@param scn table
local function Stop(scn)
    scn.stopped = true
    scn.tick = RealBotsBench.MeterEnd(scn.meter)
    timer.Simple(TAIL, function() RealBotsTest.FinishNow("auris_load", scn) end)
end

---@param scn table
local function Begin(scn)
    if scn.params.auris then AurisIngame.Listen(scn) end
    math.randomseed(scn.params.seed)
    scn.turn, scn.probe = {}, {}
    for i = 1, scn.params.talkers do
        scn.turn[i] = i
        timer.Simple(math.random() * MEAN_CLIP / scn.params.duty, function() AurisIngame.Talk(scn, i) end)
    end
    for _, p in ipairs(player.GetAll()) do
        if #scn.probe < 4 then scn.probe[#scn.probe + 1] = {sid = p:SteamID64(), account = p:AccountID(), key = Auris and Auris.VoiceKey and Auris.VoiceKey(p)} end
    end
    scn.meter = RealBotsBench.MeterBegin()
    timer.Simple(scn.params.duration, function() Stop(scn) end)
end

---@param scn table
local function Setup(scn)
    scn.params = AurisIngame.Params()
    -- upper bound only: Stop finishes the run early through FinishNow
    scn.duration = scn.params.duration + 200
    scn.names = {}
    for i = 1, scn.params.talkers do scn.names[i] = "ai_" .. i end
    if #scn.names == 0 then scn.ids = {} Begin(scn) return end
    RealBotsTest.JoinMany(scn, scn.names, function(ok)
        scn.joined = ok
        Begin(scn)
    end)
end

RealBotsTest.Scenarios.auris_load = {duration = 1000, setup = Setup, check = function(scn)
    local out = scn.params.auris and AurisIngame.Summary(scn) or {}
    out.tick, out.texts, out.spoken_seconds = scn.tick, scn.results, scn.spoken
    out.params, out.joined, out.probe = scn.params, scn.joined, scn.probe
    out.pass = scn.tick ~= nil and (not scn.params.auris or (out.results or 0) > 0 or scn.params.talkers == 0)
    return out
end}
