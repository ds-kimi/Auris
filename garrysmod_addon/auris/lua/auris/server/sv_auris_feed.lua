-- Net boundary for end-of-voice signals from the client.
-- Only responsibility: validate sender and trigger auris flush.

util.AddNetworkString("auris_end_voice")

local warnedVoiceEndHooks = {}

-- IsValid + IsPlayer guard: during map transitions the engine can fire
-- net handlers with a NULL entity before the player is fully spawned.
---@param ply Player
---@return boolean
local function isValidSender(ply)
    if not IsValid(ply) or not ply:IsPlayer() then return false end
    return Auris.CheckRateLimit(ply)
end

net.Receive("auris_end_voice", function(_, ply)
    if not isValidSender(ply) then return end
    if not Auris.PassesFilter(ply) then return end
    -- Allows external code to intercept audio before transcription via
    -- hook.Add("Auris_VoiceEnd", ...). Returning true skips transcription.
    -- Warn once per hook name so server ops know transcription may be skipped.
    local interceptors = hook.GetTable()["Auris_VoiceEnd"]
    if interceptors then
        for name in pairs(interceptors) do
            if not warnedVoiceEndHooks[name] then
                warnedVoiceEndHooks[name] = true
                MsgC(Color(255, 165, 0), "[Auris] NOTE: hook '" .. name .. "' registered on Auris_VoiceEnd — transcription may be skipped for intercepted utterances.\n")
            end
        end
    end
    if hook.Run("Auris_VoiceEnd", ply) == true then return end
    -- Branch between backends: non-empty key flips the whole pipeline to
    -- the OpenAI HTTP path and skips the whisper.cpp worker entirely.
    if Auris._config.openai_api_key ~= "" then
        Auris.SubmitRemote(ply)
    else
        auris.Flush(Auris.VoiceKey(ply))
    end
end)
