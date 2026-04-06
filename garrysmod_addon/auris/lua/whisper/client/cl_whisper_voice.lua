-- Notify server when local player stops talking
hook.Add("PlayerEndVoice", "Whisper_EndVoice", function(ply)
    if ply ~= LocalPlayer() then return end
    net.Start("whisper_end_voice")
    net.SendToServer()
end)
