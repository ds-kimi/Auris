<p align="center">
  <img src="assets/auris.png" alt="Auris logo" width="120" height="120">
</p>

<h1 align="center">Auris</h1>

<p align="center">
  <strong>Real-time voice transcription for Garry’s Mod servers</strong><br>
  <sub>Opus · whisper.cpp · extensible submodule API</sub>
</p>

<p align="center">
  <a href="https://github.com/ds-kimi/Auris/releases"><img src="https://img.shields.io/github/v/release/ds-kimi/Auris?sort=semver&amp;style=for-the-badge&amp;logo=github&amp;label=release" alt="GitHub release"></a>
  <a href="https://github.com/ds-kimi/Auris"><img src="https://img.shields.io/github/stars/ds-kimi/Auris?style=for-the-badge&amp;logo=github" alt="GitHub stars"></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/license-MIT-blue?style=for-the-badge" alt="MIT License"></a>
  <img src="https://img.shields.io/badge/Windows-0078D6?style=for-the-badge&amp;logo=windows&amp;logoColor=white" alt="Windows">
  <img src="https://img.shields.io/badge/Garry%27s%20Mod-2581d8?style=for-the-badge" alt="Garry's Mod">
</p>

<br>

Captures player voice chat via an in-process `SV_BroadcastVoiceData` detour, decodes Opus audio, and transcribes it in real time using whisper.cpp. Auris is a silent platform — it does nothing by default except make transcriptions available. What happens with them is up to submodule addons.


---

## Demo Video

[![Auris demo video](https://img.youtube.com/vi/se-04PY7Yls/hqdefault.jpg)](https://youtu.be/se-04PY7Yls)

Watch Auris transcribe Garry's Mod voice chat in real time.

---

## Performance

Auris keeps up with a full 128-player server on CPU only. Numbers below are from a real in-game
run: srcds lin64 in WSL2 on a Ryzen 7 7800X3D (16 logical CPUs), 128 gm_realbots
bots playing real speech through the Steam voice path, ~20 % talk time (~30 audio-seconds per second),
4 workers, Silero VAD on. The bots run inside srcds, so tick and RSS figures are pessimistic.
RSS is the whole srcds process, bots included. 128 talkers, corpus and method: [docs/results/auris-perf.md](docs/results/auris-perf.md).

**English**

| Model / backend | Workers x threads | CPU-s per audio-s | Latency p95 | Dropped clips | WER | RSS | Builds |
|---|---|---|---|---|---|---|---|
| **sherpa-onnx Zipformer small** *(64-bit default)* | 4 x 1 | 0.012 | 0.20 s | 0 / 606 | 6.2 % | 1.6 GB | 64-bit |
| sherpa-onnx Moonshine tiny | 4 x 1 | 0.016 | 0.23 s | 0 / 606 | 6.3 % | 1.4 GB | 64-bit |
| **whisper `tiny_en_acft_q8_0`** *(32-bit default)* | 4 x 1 | 0.063 | 0.97 s | 0 / 606 | 9.2 % | 1.7 GB | 32 + 64-bit |
| whisper `base_en_acft_q8_0` | 4 x 2 | 0.088 | 1.5 s | 3 / 606 | 11.9 % | 1.5 GB | 32 + 64-bit |
| whisper stock `ggml-tiny.en` *(old default)* | 4 x 1 | 0.226 | 49.8 s | 179 / 606 | 24.7 % | 1.7 GB | 32 + 64-bit |

**French**

| Model / backend | Workers x threads | CPU-s per audio-s | Latency p95 | Dropped clips | WER | RSS | Builds |
|---|---|---|---|---|---|---|---|
| **whisper `base_acft_q8_0`, `language = "fr"`** *(default)* | 4 x 1 | 0.074 | 0.65 s | 3 / 560 | 11.0 % | 1.7 GB | 32 + 64-bit |
| sherpa-onnx Parakeet TDT v3 int8 | 4 x 1 | 0.115 | 18.1 s | 21 / 560 | 16.2 % | 2.5 GB | 64-bit |

Before and after at 128 talkers, same clips and load (old default = Auris before this work with the
stock Whisper model, new default = ACFT model + worker pool + VAD):

- **English**: p95 latency 60.8 s to 0.97 s (~63x), 51 % of clips lost to none, Auris CPU ~1.9x lower
  while transcribing twice as many clips, WER 11.2 % to 9.2 %. With Zipformer: ~9x less CPU, WER 6.2 %.
- **French**: p95 latency 72 s to 0.65 s, 213 of 557 clips answered to 557, WER 20.9 % to 11.0 %.

Auris adds about 0.5 ms per tick on the main thread at 128 talkers. WER comes from a few dozen
utterances, so treat it as indicative.

---

## Requirements

- [Git](https://git-scm.com/)
- [Vulkan SDK](https://vulkan.lunarg.com/) 1.4.341.1 or newer *(GPU builds only)*
- [Visual Studio](https://visualstudio.microsoft.com/) with C++ build tools
- whisper.cpp model file (see "Models" below)

---

## Models

Pick the model for your language and CPU. All figures are measured at 128 talkers, see [Performance](#performance).

| Server | English | French / other languages |
|---|---|---|
| 64-bit, any size | `backend = "onnx"` + Zipformer small (or Moonshine tiny) | whisper `base_acft_q8_0` + `language = "fr"` |
| 32-bit | whisper `tiny_en_acft_q8_0` | whisper `base_acft_q8_0` + `language = "fr"` (budget 2x the CPU of 64-bit) |
| 64 players or fewer, best quality | whisper `base_en_acft_q8_0` | `base_acft_q8_0` |
| With a GPU build or the OpenAI backend | whisper `small_acft_q8_0` | same |

### Whisper (every build)

Use the **ACFT** models from [FUTO](https://voiceinput.futo.org/): they are fine-tuned to work with a shortened
audio context, so Auris shrinks the encoder to the clip length (`dynamic_audio_ctx`) and the cost drops 3.5x on
short voice clips. Stock models gain nothing from it, so the context is only shortened when the filename contains `acft`.

- `https://voiceinput.futo.org/VoiceInput/tiny_en_acft_q8_0.bin` (weak hosts, 32-bit)
- `https://voiceinput.futo.org/VoiceInput/base_en_acft_q8_0.bin` (English default)
- `https://voiceinput.futo.org/VoiceInput/base_acft_q8_0.bin` (multilingual, French: set `language = "fr"`)
- `https://voiceinput.futo.org/VoiceInput/small_acft_q8_0.bin` (only with a GPU build or the OpenAI backend)

Place the file in `garrysmod/data/auris/`. Plain `ggml-*.bin` models from
[ggerganov/whisper.cpp](https://huggingface.co/ggerganov/whisper.cpp/tree/main) still work but cost far more CPU.

> **Upgrading from a previous release?** Move your model file from `data/whisper/` to `data/auris/`, and
> set `model` in `config.lua` if you keep a different filename (the default is now `base_en_acft_q8_0.bin`).

### Voice activity detection (Silero)

Download [`ggml-silero-v6.2.0.bin`](https://huggingface.co/ggml-org/whisper-vad/blob/main/ggml-silero-v6.2.0.bin)
into `garrysmod/data/auris/`. Clips with no speech are skipped and the rest are cropped to the spoken part, which
saves CPU on open mics and breathing. If the file is missing the gate is off. Keep it on.

### ONNX (64-bit module only)

Set `backend = "onnx"`, `onnx_model_type` and `onnx_model_dir`. Models come from the
[sherpa-onnx ASR releases](https://github.com/k2-fsa/sherpa-onnx/releases/tag/asr-models) (drop the extracted folder in `garrysmod/data/auris/`):

| `onnx_model_type` | Model | Languages | Notes |
|---|---|---|---|
| `zipformer` | `sherpa-onnx-zipformer-small-en-2023-06-26` | English | cheapest and most accurate here, upper case without punctuation |
| `moonshine` | `sherpa-onnx-moonshine-tiny-en-quantized-2026-02-27` | English | loops on clips around 1 s |
| `parakeet` | `sherpa-onnx-nemo-parakeet-tdt-0.6b-v3-int8` | 25 European languages incl. French | punctuation, 1.5x the CPU and RAM of whisper base |

NVIDIA Parakeet TDT v3 is licensed [CC-BY-4.0](https://creativecommons.org/licenses/by/4.0/); keep the attribution
if you redistribute it. A 32-bit module prints an error and keeps whisper.

### 128-player capacity

These are estimates from the table above, not guarantees. 128 players at ~20 % talk time is ~30 audio-s/s, so the
workers must cover talk rate x CPU-s per audio-s with headroom:

- Zipformer or Moonshine need about half a core, `tiny_en_acft` about 2, `base_acft` (FR) about 2 to 3, `base_en_acft` about 4 (use 4 workers x 2 threads), Parakeet about 3.5.
- Prefer more workers over more threads per worker: short clips scale better that way. Keep `workers x threads` below your
  physical cores so the game tick keeps its own core. `workers = 0` picks half the cores, at most 4.
- `max_queue_seconds = 30` drops the oldest clips instead of letting latency grow. `affinity = true` keeps workers off the
  main-thread core; `auto_workers = true` lets the pool grow and shrink with the queue.
- On a saturated host use a smaller model, an ONNX backend, a GPU build, or the OpenAI backend (no local compute).

If you want to use a different model name or path, update it in:

- `garrysmod_addon/auris/lua/auris/config.lua`

---

## Building the module

See **[BUILD.md](BUILD.md)** for the full build guide — all platforms, architectures, CPU vs GPU variants, and flag reference.

**Quick start (Windows, CPU-only):**

```
RUN_ONCE.bat
premake5.exe --os=windows --gmcommon=./garrysmod_common vs2022
```

Open `projects/windows/vs2022/` and build in Release.

---

## Tested on

- Windows 11
- Vulkan SDK 1.4.341.1
- Visual Studio 18.5.11626.173

---

## Configuration

Edit `garrysmod_addon/auris/lua/auris/config.lua` to change the model path, language, thread count, and other options. Every key can also be overridden at runtime via ConVar (`auris_threads`, `auris_language`, `auris_debug`) in `server.cfg` — ConVar values take precedence over the file.

---

## Backends

Auris has two transcription backends. Both fire the same `Auris_Transcription` hook with identical arguments, so submodules work unchanged on either.

| Backend | When it runs | Trade-off |
|---|---|---|
| **Local (whisper.cpp)** | `openai_api_key = ""` (default) | Private, offline, no per-clip cost; uses server CPU/GPU and requires a downloaded model file |
| **Remote (OpenAI)** | `openai_api_key` set in `config.lua` | Zero local compute, hosted accuracy; requires network I/O and incurs OpenAI per-minute billing |

To switch to the remote backend, fill the two keys in `config.lua`:

```lua
openai_api_key = "sk-...",
openai_model   = "whisper-1", -- or gpt-4o-mini-transcribe / gpt-4o-transcribe
```

Whisper.cpp is not loaded and the worker thread is not spawned in remote mode — the Steam voice detour still runs, captured Opus is still decoded to float32 PCM, but transcription happens entirely at `https://api.openai.com/v1/audio/transcriptions`. See [OpenAI's speech-to-text docs](https://developers.openai.com/api/docs/guides/speech-to-text?lang=curl) for pricing and model options.

---

## Submodule API

Auris exposes a hook that any addon can listen to:

```lua
hook.Run("Auris_Transcription", ply, steamid64, text, audio)
```

| Parameter | Type | Notes |
|---|---|---|
| `ply` | `GPlayer` or `nil` | `nil` if the player disconnected before the result arrived |
| `steamid64` | `string` | Always present |
| `text` | `string` | Transcribed speech |
| `audio` | `string` or `nil` | Raw 16 kHz mono float32 PCM binary. Convert to a playable WAV with `Auris.PCMToWAV(audio)` |

### Quick start

Create a new GMod addon with one server-side file:

```lua
-- lua/autorun/server/sv_myaddon_init.lua

-- timer.Simple(0) defers until all autorun files have loaded,
-- avoiding load-order issues with Auris.
timer.Simple(0, function()
    if not Auris then
        ErrorNoHalt("[myAddon] Auris core not found\n")
        return
    end

    Auris.Subscribe("MyAddon_Feature", function(ply, steamid64, text, audio)
        local name = IsValid(ply) and ply:Nick() or "Disconnected"
        Msg("[myAddon] " .. name .. ": " .. text .. "\n")

        -- optional: save the voice clip to disk
        if audio then
            file.Write("myaddon/" .. steamid64 .. ".wav", Auris.PCMToWAV(audio))
        end
    end)
end)
```

### Full API reference

See [API.md](API.md) for the complete API, subscriber name conventions, version guards, and the publishing checklist to get your submodule listed here.

### Community submodules

| Addon | Description | Author |
|---|---|---|
| [auris-logger](garrysmod_addon/auris-logger/) | Prints every transcription to the server console with player name and SteamID64 | [ds-kimi](https://github.com/ds-kimi) |
| [auris-discord](garrysmod_addon/auris-discord/) | Forwards every transcription to a Discord webhook (requires [gmsv_reqwest](https://github.com/williamvenner/gmsv_reqwest)) | [ds-kimi](https://github.com/ds-kimi) |
| [auris-subtitles](garrysmod_addon/auris-subtitles/) | Displays transcriptions as animated worldspace subtitles above the speaker's head, visible to nearby players | [ds-kimi](https://github.com/ds-kimi) |

To add yours: follow the [publishing guide in API.md](API.md#publishing-your-submodule), then open a [Submit Submodule](https://github.com/ds-kimi/Auris/issues/new?template=submodule.yml) issue.

---

## Star history

<a href="https://www.star-history.com/#ds-kimi/Auris&amp;Date"><img src="https://api.star-history.com/svg?repos=ds-kimi/Auris&amp;type=Date" alt="Star history chart"></a>

See the interactive chart on **[Star History](https://www.star-history.com/)** (repo: `ds-kimi/Auris`).
