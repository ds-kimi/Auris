-- Run parameters written by tools/ingame/run_case.sh; clamped because a
-- typo in a matrix row must not spawn 10 000 bots or loop forever.

AurisIngame = AurisIngame or {}

local PARAMS = "auris_ingame/params.json"
local MAX_CLIPS = 32

---@class AurisIngameParams
---@field talkers integer bots that speak (0 = idle server, tick baseline)
---@field duty number fraction of time each talker speaks
---@field duration number measured seconds after every bot joined
---@field clips string[] WAV paths under data/
---@field seed integer
---@field auris boolean false when Auris is not installed (baseline)

---@return AurisIngameParams
function AurisIngame.Params()
    local raw = util.JSONToTable(file.Read(PARAMS, "DATA") or "") or {}
    local clips = {}
    for _, c in ipairs(istable(raw.clips) and raw.clips or {}) do
        if isstring(c) and #clips < MAX_CLIPS and c:match("^[%w_/%.%-]+%.wav$") then clips[#clips + 1] = c end
    end
    return {
        talkers = math.Clamp(math.floor(tonumber(raw.talkers) or 1), 0, 128),
        duty = math.Clamp(tonumber(raw.duty) or 0.2, 0.01, 1),
        duration = math.Clamp(tonumber(raw.duration) or 60, 5, 900),
        clips = clips,
        seed = math.floor(tonumber(raw.seed) or 1),
        auris = raw.auris ~= false,
    }
end
