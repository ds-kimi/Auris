-- Receive end-voice signal from client and flush audio
util.AddNetworkString("whisper_end_voice")

net.Receive("whisper_end_voice", function(len, ply)
    if not IsValid(ply) then return end
    whisper.FlushAll()
    if whisper.IsDebug() then
        print("[Whisper] Flushing for " .. ply:Nick())
    end
end)
