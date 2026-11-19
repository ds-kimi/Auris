# Contributing

Thanks for contributing to Auris.

## Reporting bugs

- Use the Bug report issue template.
- Include clear repro steps and relevant server logs.
- Include your OS, GPU, Vulkan SDK version, and model file.

## Requesting features

- Use the Feature request issue template.
- Keep requests focused: describe the problem first, then the proposed solution.

## Pull requests

- Keep PRs small and scoped.
- Prefer incremental changes over large refactors.
- Update documentation if behavior or configuration changes.

## Development notes

- Windows builds are generated via Premake.
- See README for build and configuration details.
- Sources live in subsystem folders under `source/`: `core/` (module entry, Lua bindings, config), `audio/` (Opus, Steam voice, buffers), `hook/` (voice detour), `transcribe/` (pool, queue, workers, whisper params), `transcribe/onnx/` (ONNX backend), `vad/` (Silero gate), `platform/` (CPU topology, thread placement), `util/` (debug log). Includes are relative to `source/`, for example `#include "core/auris_config.h"`. New files are picked up by premake automatically.
