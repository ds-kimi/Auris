-- Poll whisper for transcriptions and log them
local function formatTimestamp()
    return os.date("[%H:%M:%S]")
end

local function getPlayerBySteamID64(sid)
    for _, ply in ipairs(player.GetAll()) do
        if ply:SteamID64() == sid then return ply end
    end
    return nil
end

local function writeTranscript(sid, text)
    local ply = getPlayerBySteamID64(sid)
    local name = IsValid(ply) and ply:Nick() or "Unknown"
    local steamid = IsValid(ply) and ply:SteamID() or sid
    local line = formatTimestamp() .. " " .. name
    line = line .. " (" .. steamid .. "): " .. text .. "\n"

    file.Append("whisper/transcript.txt", line)
    print("[Whisper] " .. name .. ": " .. text)
end

timer.Create("Whisper_Poll", 0.1, 0, function()
    local sid, text = whisper.Poll()
    while sid do
        writeTranscript(sid, text)
        sid, text = whisper.Poll()
    end
end)