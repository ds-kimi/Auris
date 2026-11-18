#!/usr/bin/env bash
set -euo pipefail

# ============================================================
# RUN_ONCE.sh
# Full setup from a fresh git clone. Run from the repo root.
#
# Requirements (full/Vulkan):
#   - Git, CMake, g++ (C++17)
#   - Vulkan SDK (glslc in PATH or VULKAN_SDK set)
#
# Requirements (--cpu-only):
#   - Git, CMake, g++ (C++17)
#   - No Vulkan SDK needed
# ============================================================

CPU_ONLY=0
for arg in "$@"; do
    [[ "$arg" == "--cpu-only" ]] && CPU_ONLY=1
done

if [[ "$CPU_ONLY" -eq 0 ]]; then
    read -r -p "GPU support (Vulkan)? [y/n]: " gpu_ans
    [[ "$gpu_ans" =~ ^[Nn] ]] && CPU_ONLY=1
fi

MODULE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$MODULE_DIR"
VENDOR_WHISPER_DIR="$MODULE_DIR/vendor/whisper.cpp"
GMCOMMON_DIR="$REPO_ROOT/garrysmod_common"
SHADERS_DIR="$VENDOR_WHISPER_DIR/ggml/src/ggml-vulkan/vulkan-shaders"
GEN_SRC="$SHADERS_DIR/vulkan-shaders-gen.cpp"
GEN_EXE="$MODULE_DIR/vulkan-shaders-gen"
SPV_DIR="$MODULE_DIR/shader_spv"
CPP_DIR="$MODULE_DIR/shader_cpp"
OUT_CPP="$VENDOR_WHISPER_DIR/ggml/src/ggml-vulkan/ggml-vulkan-shaders.cpp"
OUT_HPP="$VENDOR_WHISPER_DIR/ggml/src/ggml-vulkan/ggml-vulkan-shaders.hpp"
VULKAN_CPP="$VENDOR_WHISPER_DIR/ggml/src/ggml-vulkan/ggml-vulkan.cpp"

if [[ "$CPU_ONLY" -eq 1 ]]; then
    echo "[mode] CPU-only — Vulkan steps will be skipped"
else
    echo "[mode] Full Vulkan build"
fi

# ── [1/6] Check dependencies ─────────────────────────────────
echo "[1/6] Checking dependencies..."
for cmd in git cmake g++; do
    if ! command -v "$cmd" &>/dev/null; then
        echo "ERROR: '$cmd' not found in PATH." >&2
        exit 1
    fi
done

if [[ "$CPU_ONLY" -eq 0 ]]; then
    GLSLC=""
    if command -v glslc &>/dev/null; then
        GLSLC="glslc"
    elif [[ -n "${VULKAN_SDK:-}" && -x "$VULKAN_SDK/bin/glslc" ]]; then
        GLSLC="$VULKAN_SDK/bin/glslc"
    else
        # Common Linux install paths
        for candidate in /usr/bin/glslc /usr/local/bin/glslc; do
            if [[ -x "$candidate" ]]; then GLSLC="$candidate"; break; fi
        done
    fi
    if [[ -z "$GLSLC" ]]; then
        echo "ERROR: glslc not found. Install Vulkan SDK or pass --cpu-only." >&2
        exit 1
    fi
    echo "[glslc] Using: $GLSLC"
fi

# ── [2/6] Git submodules ──────────────────────────────────────
echo "[2/6] Initialising git submodules..."
pushd "$REPO_ROOT" >/dev/null
git submodule update --init --recursive
popd >/dev/null

# ── [3/6] garrysmod_common submodules ────────────────────────
echo "[3/6] Initialising garrysmod_common submodules..."
pushd "$GMCOMMON_DIR" >/dev/null
git submodule update --init --recursive
popd >/dev/null

# 64-bit builds link sherpa-onnx (ONNX backend); 32-bit builds never use it
echo "[sherpa-onnx] Fetching pinned static libs..."
bash "$MODULE_DIR/tools/setup/fetch_sherpa_onnx.sh"

# ── [4/6] Build whisper.cpp ───────────────────────────────────
pushd "$VENDOR_WHISPER_DIR" >/dev/null
if [[ "$CPU_ONLY" -eq 1 ]]; then
    echo "[4/6] Building whisper.cpp (CPU only)..."
    cmake -B build
else
    echo "[4/6] Building whisper.cpp with Vulkan (this takes a while)..."
    cmake -B build -DGGML_VULKAN=1
fi
cmake --build build -j"$(nproc)" --config Release
popd >/dev/null

# Patch quants.c — _pdep_u64 is x86_64-only; 32-bit builds must use the scalar fallback.
QUANTS_C="$VENDOR_WHISPER_DIR/ggml/src/ggml-cpu/arch/x86/quants.c"
if grep -q '#ifdef __BMI2__' "$QUANTS_C" 2>/dev/null; then
    echo "[patch] Patching quants.c for 32-bit BMI2..."
    sed -i 's/#ifdef __BMI2__/#if defined(__BMI2__) \&\& defined(__x86_64__)/g' "$QUANTS_C"
    echo "[patch] Done."
else
    echo "[patch] quants.c already patched, skipping."
fi

if [[ "$CPU_ONLY" -eq 1 ]]; then
    echo ""
    echo "Setup complete (CPU only). Now run premake to generate the build files."
    exit 0
fi

# ── [5/6] Build vulkan-shaders-gen ───────────────────────────
echo "[5/6] Building vulkan-shaders-gen..."
g++ -std=c++17 -O2 -o "$GEN_EXE" "$GEN_SRC"

# ── [6/6] Compile shaders, merge, patch ──────────────────────
echo "[6/6] Compiling shaders..."
rm -rf "$SPV_DIR" "$CPP_DIR"
mkdir -p "$SPV_DIR" "$CPP_DIR"

"$GEN_EXE" --output-dir "$SPV_DIR" --target-hpp "$OUT_HPP"

COUNT=0
for COMP in "$SHADERS_DIR"/*.comp; do
    NAME="$(basename "$COMP" .comp)"
    "$GEN_EXE" --glslc "$GLSLC" --source "$COMP" \
        --output-dir "$SPV_DIR" --target-hpp "$OUT_HPP" \
        --target-cpp "$CPP_DIR/$NAME.cpp"
    COUNT=$((COUNT + 1))
done
echo "Compiled $COUNT shaders."

echo "Merging shaders..."
rm -f "$OUT_CPP"
cat "$CPP_DIR"/*.cpp > "$OUT_CPP"

rm -rf "$SPV_DIR" "$CPP_DIR" "$GEN_EXE"

# Patch ggml-vulkan.cpp — adds missing ostream operator on MSVC,
# harmless to apply on Linux too so both platforms stay in sync.
if ! grep -q "operator<<(std::ostream" "$VULKAN_CPP" 2>/dev/null; then
    echo "[patch] Patching ggml-vulkan.cpp..."
    INJECT=$'#ifdef _MSC_VER\n#include <ostream>\ninline std::ostream& operator<<(std::ostream& os, vk::Buffer const&) { return os; }\n#endif'
    sed -i "s|#include <vulkan/vulkan.hpp>|#include <vulkan/vulkan.hpp>\n\n${INJECT}|" "$VULKAN_CPP"
    echo "[patch] Done."
else
    echo "[patch] Already patched, skipping."
fi

echo ""
echo "Setup complete. Now run premake to generate the build files."
