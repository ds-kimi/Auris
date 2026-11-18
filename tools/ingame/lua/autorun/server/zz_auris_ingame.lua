-- Auris in-game load test, installed next to the gm_realbots harness by
-- tools/ingame/install.sh. Never shipped with the Auris addon.
RealBotsTest = RealBotsTest or {}
RealBotsTest.Scenarios = RealBotsTest.Scenarios or {}

include("auris_ingame/sv_params.lua")
include("auris_ingame/sv_collect.lua")
include("auris_ingame/sv_talk.lua")
include("auris_ingame/sv_scenario.lua")
