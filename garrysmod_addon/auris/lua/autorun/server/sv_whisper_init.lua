AddCSLuaFile("autorun/client/cl_whisper_init.lua")
AddCSLuaFile("whisper/client/cl_whisper_voice.lua")

require("eightbit")
require("whisper")

if not file.IsDir("whisper", "DATA") then
    file.CreateDir("whisper")
end

-- Load config before anything else
include("whisper/server/sv_whisper_config.lua")

-- Enable 8bit broadcast to localhost for voice data
eightbit.SetBroadcastIP("127.0.0.1")
eightbit.SetBroadcastPort(WHISPER_PORT)
eightbit.EnableBroadcast(true)
print("[Whisper] 8bit broadcast on port " .. WHISPER_PORT)

if not whisper.Init(WHISPER_MODEL) then
    print("[Whisper] Failed to load: " .. WHISPER_MODEL)
    return
end

-- Apply config to the C++ module
ApplyWhisperConfig()

whisper.Listen(WHISPER_PORT)
print("[Whisper] Listening on port " .. WHISPER_PORT)

include("whisper/server/sv_whisper_feed.lua")
include("whisper/server/sv_whisper_log.lua")

print("[Whisper] Addon loaded")
