#!/usr/bin/env bash
# One matrix cell: edits the installed Auris config.lua, writes the scenario
# params, runs auris_load and copies the JSON to $OUT/<label>.json.
# Usage: run_case.sh <label> <talkers> <clips_csv> [key=value ...]
#   values are Lua literals: model='"garrysmod/data/auris/x.bin"' threads=2
set -uo pipefail
label="$1"; talkers="$2"; clips="$3"; shift 3
[[ "$label" =~ ^[A-Za-z0-9_.-]{1,64}$ ]] || { echo "bad label" >&2; exit 2; }
[[ "$talkers" =~ ^[0-9]{1,3}$ ]] || { echo "bad talkers" >&2; exit 2; }
realbots="${REALBOTS:-/mnt/c/Users/Kimi/Documents/gm_realbots}"
gm="$HOME/realbots_srv/lin64/garrysmod"
out="${OUT:-$HOME/auris_i2/results}"
cfg="$gm/addons/auris/lua/auris/config.lua"
mkdir -p "$out"
auris=true; [ -f "$cfg" ] || auris=false
# every cell starts from the shipped defaults, not from the previous cell
if $auris; then cp "$(dirname "$0")/../../garrysmod_addon/auris/lua/auris/config.lua" "$cfg"; fi
for kv in "$@"; do
    key="${kv%%=*}"; val="${kv#*=}"
    [[ "$key" =~ ^[a-z_]{1,32}$ ]] || { echo "bad key $key" >&2; exit 2; }
    grep -qE "^ *$key *=" "$cfg" || { echo "unknown key $key" >&2; exit 2; }
    sed -i -E "s|^( *$key *= *).*,|\1$val,|" "$cfg"
done
list="$(echo "$clips" | sed -E 's/([^,]+)/"auris_clips\/\1.wav"/g')"
printf '{"talkers": %s, "duty": %s, "duration": %s, "seed": 7, "auris": %s, "clips": [%s]}\n' \
    "$talkers" "${DUTY:-0.2}" "${DURATION:-60}" "$auris" "$list" > "$gm/data/auris_ingame/params.json"
bash "$realbots/tools/server/run_scenario.sh" auris_load "${TIMEOUT:-400}" "${TICK:-66}" 128 > /dev/null
rc=$?
# run_scenario.sh also copies the JSON into the realbots checkout: not ours to keep
rm -f "$realbots"/docs/results/linux64/auris_load*.json
cp "$gm/data/realbots_tests/auris_load.json" "$out/$label.json" 2>/dev/null
grep -hE "\[Auris\].*(rror|fail|Fail)" "$gm/console.log" | sort | uniq -c | head -5
echo "$label rc=$rc"
