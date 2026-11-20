AddCSLuaFile("auris/client/cl_auris_voice.lua")

-- file.Find with wildcard covers platform suffixes (win64, linux64, etc.)
-- avoids the engine-level "File not found" print that pcall cannot suppress.
local function moduleExists(name)
    local files = file.Find("bin/gmsv_" .. name .. "_*.dll", "LUA")
    return #files > 0
end

local gpuOk = moduleExists("auris-gpu") and pcall(require, "auris-gpu")
if gpuOk and auris then
    MsgC(Color(100, 220, 100), "[Auris] GPU backend loaded\n")
elseif moduleExists("auris") and pcall(require, "auris") then
    MsgC(Color(200, 200, 200), "[Auris] CPU-only backend loaded\n")
else
    for _ = 1, 5 do
        MsgC(Color(255, 80, 80), "[Auris] ERROR: Module not found. Did you install gmsv_auris_* / gmsv_auris-gpu_*.dll into garrysmod/lua/bin/?\n")
    end
    return
end

-- Version + api table first so later server files can reference the Auris global.
include("auris/server/sv_auris_version.lua")
include("auris/server/sv_auris_api.lua")

include("auris/server/sv_auris_config.lua")
Auris.LoadConfig()

include("auris/server/sv_auris_sanitize.lua")
include("auris/server/sv_auris_sanitize_cpu.lua")
include("auris/server/sv_auris_boot.lua")
Auris.Boot()

-- Player lookup must load before feed/openai and poll in case a result
-- arrives before the player lookup function exists.
include("auris/server/sv_auris_player.lua")
include("auris/server/sv_auris_openai.lua")
include("auris/server/sv_auris_feed.lua")

include("auris/server/sv_auris_poll.lua")
Auris.StartPollLoop()
