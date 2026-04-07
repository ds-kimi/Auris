-- Poll whisper for transcriptions and log them
if not whisper.IsDebug() then return end

local function formatTimestamp()
    return os.date("[%H:%M:%S]")
end

local function writeTranscript(sid, text)
    local ply = player.GetBySteamID64(sid)
    local name = IsValid(ply) and ply:Nick() or "Unknown"
    local line = ("%s %s (%s): %s\n"):format(formatTimestamp(), name, sid, text)

    file.Append("whisper/transcript.txt", line)
    print("[Whisper] " .. name .. ": " .. text)
end

timer.Create("Whisper_Poll", 0.1, 0, function()
    if not whisper.IsDebug() then return end

    local sid, text = whisper.Poll()
    while sid do
        writeTranscript(sid, text)
        sid, text = whisper.Poll()
    end
end)

whisper.Debug(true)