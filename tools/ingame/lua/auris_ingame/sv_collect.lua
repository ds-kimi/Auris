-- Results side: one record per flushed clip, matched to the Auris result by
-- speaker entity (FIFO per bot, a talker never has two clips in flight;
-- realbots all share SteamID64 "0", so the SteamID cannot tell them apart).

AurisIngame = AurisIngame or {}

---@param scn table
function AurisIngame.Listen(scn)
    scn.results, scn.pending = {}, {}
    Auris.Subscribe("auris_ingame", function(ply, _, text)
        local sid = IsValid(ply) and ply:EntIndex() or -1
        local queue = scn.pending[sid]
        local rec = queue and table.remove(queue, 1)
        if not rec then scn.unmatched = (scn.unmatched or 0) + 1 return end
        rec.latency_ms = (SysTime() - rec.flushed) * 1000
        rec.text = text
        rec.flushed = nil
        scn.results[#scn.results + 1] = rec
    end)
end

---@param scn table
---@param sid integer speaker entity index
---@param clip string
function AurisIngame.Expect(scn, sid, clip)
    scn.flushes = (scn.flushes or 0) + 1
    scn.pending[sid] = scn.pending[sid] or {}
    table.insert(scn.pending[sid], {clip = clip, sid = sid, flushed = SysTime()})
end

---@param list number[]
---@param q number
---@return number
local function Quantile(list, q)
    if #list == 0 then return 0 end
    table.sort(list)
    return list[math.Clamp(math.ceil(#list * q), 1, #list)]
end

---@param scn table
---@return table
function AurisIngame.Summary(scn)
    local lat = {}
    for _, r in ipairs(scn.results or {}) do lat[#lat + 1] = r.latency_ms end
    local stats = auris and auris.GetStats and auris.GetStats() or {}
    return {flushes = scn.flushes or 0, results = #lat, unmatched = scn.unmatched or 0,
        latency_p50_ms = Quantile(lat, 0.5), latency_p95_ms = Quantile(lat, 0.95),
        latency_max_ms = Quantile(lat, 1), dropped = stats.dropped, jobs = stats.jobs,
        cpu_per_audio = stats.cpu_per_audio, audio_seconds = stats.audio_seconds,
        worker_cpu_ms = stats.cpu_ms, active_workers = stats.active_workers}
end
