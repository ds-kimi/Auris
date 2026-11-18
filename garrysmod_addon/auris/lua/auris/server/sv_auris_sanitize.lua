-- Sanitisers for operator-supplied config values, shared by the boot step.
-- Kept apart from sv_auris_boot.lua to respect the 4-functions-per-file rule.

-- Values come from an operator file and ConVars: a typo like workers = 64
-- would pin every core, so out-of-range or non-numeric values are clamped.
---@param val any
---@param min number
---@param max number
---@param default number
---@return number
local function clampNumber(val, min, max, default)
    val = tonumber(val)
    if not val or val ~= val then return default end
    return math.Clamp(val, min, max)
end

-- The path goes straight to fopen in C++: anything but a plain relative .bin
-- path falls back to the default instead of probing arbitrary files.
---@param path any
---@return string
local function resolveVadModel(path)
    local default = "garrysmod/data/auris/ggml-silero-v6.2.0.bin"
    if not isstring(path) then return default end
    if path == "" then return "" end
    if #path > 260 or not string.match(path, "^[%w_%-%./]+%.bin$") then return default end
    if string.find(path, "..", 1, true) then return default end
    return path
end

Auris._sanitize = {
    clampNumber = clampNumber,
    resolveVadModel = resolveVadModel,
}
