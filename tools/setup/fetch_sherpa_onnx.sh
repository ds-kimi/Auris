#!/usr/bin/env bash
# Fetches the pinned sherpa-onnx static libs (with static ONNX Runtime) for the
# 64-bit Linux build into vendor/sherpa-onnx. Idempotent; checks SHA256.
set -euo pipefail

VERSION="v2.0.1"
LIB_SHA="4b4c36e5560696e2864da3d9a7cdbb1fc1f0ce7af3509e19a659341debda009a"
HDR_SHA="5698f38ba09ad2bdf085c56a6cbbd7741acf15e25f4ce5c7097b9702cadf437f"
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
DEST="$ROOT/vendor/sherpa-onnx"
BASE="https://github.com/k2-fsa/sherpa-onnx/releases/download/$VERSION"
TAR="sherpa-onnx-$VERSION-linux-x64-static-no-tts-lib.tar.bz2"
HDR="$DEST/include/sherpa-onnx/c-api/c-api.h"

if [[ -f "$DEST/linux-x64/lib/libsherpa-onnx-c-api.a" && -f "$HDR" ]]; then
    echo "[sherpa-onnx] already present, skipping"
    exit 0
fi
mkdir -p "$DEST/dl" "$DEST/linux-x64" "$(dirname "$HDR")"
curl -fsSL -o "$DEST/dl/$TAR" "$BASE/$TAR"
echo "$LIB_SHA  $DEST/dl/$TAR" | sha256sum -c -
tar -xjf "$DEST/dl/$TAR" -C "$DEST/linux-x64" --strip-components=1
curl -fsSL -o "$HDR" "https://raw.githubusercontent.com/k2-fsa/sherpa-onnx/$VERSION/sherpa-onnx/c-api/c-api.h"
echo "$HDR_SHA  $HDR" | sha256sum -c -
echo "[sherpa-onnx] $VERSION ready"
