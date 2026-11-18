# Building Auris

> **First time?** Run `RUN_ONCE.bat` (Windows) or `./RUN_ONCE.sh` (Linux) from the repo root before anything else — it compiles Vulkan shaders and patches vendor files.

---

## Table of Contents

- [Flags Reference](#flags-reference)
- [Windows — Visual Studio](#windows--visual-studio)
- [Linux — GNU Make](#linux--gnu-make)
- [Output Locations](#output-locations)
- [Benchmark (auris-bench)](#benchmark-auris-bench)

---

## Flags Reference

| Flag | Default | Description |
|---|---|---|
| `--gmcommon=<path>` | *(required)* | Path to your `garrysmod_common` clone |
| `--with-vulkan` | off | Enable Vulkan GPU backend. Requires Vulkan SDK. Without it: CPU-only, no `libvulkan` dependency |
| `--generator-version=3` | `2` | Enable x86-64 platform support (needed for 64-bit builds) |
| `--os=windows` / `--os=linux` | host OS | Target platform |

---

## Windows — Visual Studio

### Prerequisites

- Visual Studio 2022 with **Desktop development with C++**
- [Vulkan SDK](https://vulkan.lunarg.com/) 1.4.341.1+ *(GPU builds only)*

---

### CPU-only · 32-bit

```bat
premake5.exe --os=windows --gmcommon=./garrysmod_common vs2022
```

Open `projects/windows/vs2022/` → build **Release | Win32**

---

### CPU-only · 64-bit

```bat
premake5.exe --os=windows --gmcommon=./garrysmod_common --generator-version=3 vs2022
```

Open `projects/windows/vs2022/` → build **Release | x64**

---

### GPU (Vulkan) · 32-bit

```bat
premake5.exe --os=windows --gmcommon=./garrysmod_common --with-vulkan vs2022
```

Open `projects/windows/vs2022/` → build **Release | Win32**

> The 32-bit Vulkan import lib lives in `vendor/vulkan/lib32/`. stdcall symbol aliasing is handled automatically in `premake5.lua`.

---

### GPU (Vulkan) · 64-bit

```bat
premake5.exe --os=windows --gmcommon=./garrysmod_common --with-vulkan --generator-version=3 vs2022
```

Open `projects/windows/vs2022/` → build **Release | x64**

> Requires `VULKAN_SDK` environment variable set, or the SDK installed at `C:/VulkanSDK/1.4.341.1`.

---

## Linux — GNU Make

### Prerequisites

- GCC or Clang with C++17 support
- `make`, `premake5`
- `libvulkan-dev` *(GPU builds only)*

---

### CPU-only · 32-bit

```sh
./premake5 --os=linux --gmcommon=./garrysmod_common gmake2
cd projects/linux/make
make
```

---

### CPU-only · 64-bit

```sh
./premake5 --os=linux --gmcommon=./garrysmod_common --generator-version=3 gmake2
cd projects/linux/make
make config=release_x86_64
```

---

### GPU (Vulkan) · 32-bit

```sh
./premake5 --os=linux --gmcommon=./garrysmod_common --with-vulkan gmake2
cd projects/linux/make
make
```

---

### GPU (Vulkan) · 64-bit

```sh
./premake5 --os=linux --gmcommon=./garrysmod_common --with-vulkan --generator-version=3 gmake2
cd projects/linux/make
make config=release_x86_64
```

---

## Benchmark (`auris-bench`)

`auris-bench` is a console tool built from the same whisper/ggml sources and flags as the module. It measures
CPU-s per audio-s, latency p50/p95 and peak RSS on your own hardware, which is how to size `workers`, `threads` and the model
before going live. It is generated with the module by premake: build the `auris-bench` project in the same
configuration (for example `MSBuild projects/windows/vs2022/auris-bench.vcxproj -p:Configuration=Release -p:Platform=x64`,
or `make config=release_x86_64 auris-bench` on Linux).

```sh
auris-bench -m tiny_en_acft_q8_0.bin -w clips/ -j 4 -r 3 --vad ggml-silero-v6.2.0.bin
```

Input is 16 kHz mono WAV (a file or a folder). 64-bit builds also take `--onnx <dir> --onnx-model-type zipformer`.
See [tools/bench/README.md](tools/bench/README.md) for every flag. Reference results are in
[docs/results/auris-perf.md](docs/results/auris-perf.md).

---

## Output Locations

| Platform | Architecture | Module name (CPU) | Module name (GPU) |
|---|---|---|---|
| Windows | 32-bit | `gmsv_auris_win32.dll` | `gmsv_auris-gpu_win32.dll` |
| Windows | 64-bit | `gmsv_auris_win64.dll` | `gmsv_auris-gpu_win64.dll` |
| Linux | 32-bit | `gmsv_auris_linux.dll` | `gmsv_auris-gpu_linux.dll` |
| Linux | 64-bit | `gmsv_auris_linux64.dll` | `gmsv_auris-gpu_linux64.dll` |

Place the built `.dll` in your server's `garrysmod/lua/bin/` directory.

---

> **CPU vs GPU?** CPU builds run on any machine including headless dedicated servers with no GPU.
> GPU (Vulkan) builds are faster but require a Vulkan-capable GPU on the server host.
