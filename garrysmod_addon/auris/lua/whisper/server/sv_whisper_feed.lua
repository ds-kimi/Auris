-- Receive end-voice signal from client and flush audio

local function unLoadPlayer(pPlayer)

    whisper.Flush(pPlayer:SteamID64())
    if whisper.IsDebug() then

        print("[Whisper] Flushing for " .. pPlayer:Nick())

    end

end

timer.Create("Whisper::CheckVoice", 0.25, 0, function()

    if player.GetCount() == 0 then return end
    
    for _, pPlayer in player.Iterator() do

        if not IsValid(pPlayer) or not pPlayer:IsPlayer() or pPlayer:IsBot() then continue end

        if pPlayer:IsSpeaking() and not pPlayer.bSpeaking then
            pPlayer.bSpeaking = true
        elseif pPlayer.bSpeaking and not pPlayer:IsSpeaking() then
            pPlayer.bSpeaking = nil
            unLoadPlayer(pPlayer)
        end

    end

end)