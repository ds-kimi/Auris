#!/usr/bin/env bash
# Installs Auris lin64 + the auris_load scenario into the gm_realbots WSL test
# server (~/realbots_srv/lin64). Models and clips are symlinked from the
# Windows checkout, the Auris addon is copied so run_case.sh can edit config.lua.
# Usage: install.sh [--no-auris]   (--no-auris: tick baseline without Auris)
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
auris="$(cd "$here/../.." && pwd)"
realbots="${REALBOTS:-/mnt/c/Users/Kimi/Documents/gm_realbots}"
gm="$HOME/realbots_srv/lin64/garrysmod"
dll="$auris/projects/linux/gmake/x86_64/Release/gmsv_auris_linux64.dll"

bash "$realbots/tools/server/make_test_server.sh" > /dev/null
mkdir -p "$gm/addons/auris_ingame" && ln -s "$here/lua" "$gm/addons/auris_ingame/lua"
mkdir -p "$gm/data/auris" "$gm/data/auris_clips" "$gm/data/auris_ingame"
for f in "$auris"/models/*.bin "$auris"/models/onnx/*; do ln -sfn "$f" "$gm/data/auris/$(basename "$f")"; done
cp "$auris"/models/clips/*.wav "$gm/data/auris_clips/"
if [ "${1:-}" != "--no-auris" ]; then
    cp "$dll" "$gm/lua/bin/"
    cp -r "$auris/garrysmod_addon/auris" "$gm/addons/auris"
fi
echo "auris test server ready"
