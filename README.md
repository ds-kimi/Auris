# Auris

Real-time voice transcription for Garry's Mod servers. Captures player voice chat via gm_8bit, decodes Opus audio, and transcribes it using whisper.cpp. Transcriptions are written to `data/whisper/transcript.txt` and printed to the server console.

## Demo Video

[![Auris demo video](https://img.youtube.com/vi/se-04PY7Yls/hqdefault.jpg)](https://youtu.be/se-04PY7Yls)

Watch Auris transcribe Garry's Mod voice chat in real time.

## Requirements

- [gm_8bit](https://github.com/Meachamp/gm_8bit) binary module
- Vulkan-capable GPU (NVIDIA or AMD)
- [Git](https://git-scm.com/)
- [CMake](https://cmake.org/)
- [Vulkan SDK](https://vulkan.lunarg.com/) 1.4.341.1 or newer
- [Visual Studio](https://visualstudio.microsoft.com/) with C++ build tools
- whisper.cpp model file placed at `data/whisper/ggml-tiny.en.bin` (or configured in `lua/whisper/server/sv_whisper_config.lua`)

## Building the module

Run the setup script once from the repo root to compile shaders and patch vendor files:

```
module\build_shaders.bat
```

Then generate the Visual Studio solution:

```
premake5.exe --os=windows --gmcommon=./garrysmod_common vs2022
```

Open the generated solution in `projects/windows/vs2022/` and build in Release.

## Tested on

- Windows 11
- Vulkan SDK 1.4.341.1
- Visual Studio 18.5.11626.173

## Configuration

Edit `lua/whisper/server/sv_whisper_config.lua` to change the model path, language, thread count, and other options.
