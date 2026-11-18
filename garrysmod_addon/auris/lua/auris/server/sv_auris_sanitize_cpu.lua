-- CPU-list sanitiser, apart from sv_auris_sanitize.lua (4-functions-per-file rule).

-- The list goes to a C parser that sets affinity bits: only digits, commas
-- and dashes reach it, and a short cap keeps a typo from becoming a huge loop.
---@param val any
---@return string
local function resolveCpuList(val)
    if not isstring(val) or #val > 64 then return "" end
    if not string.match(val, "^[%d,%-]*$") then return "" end
    return val
end

Auris._sanitize.resolveCpuList = resolveCpuList
