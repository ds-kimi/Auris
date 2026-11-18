# auris-bench

Console benchmark built from the same whisper/ggml sources, defines and flags as the Auris
module (`AddWhisperCore()` in `premake5.lua`), with the module's own param builder
(`whisper_params.cpp`, `job_limits.cpp`) and Silero gate. Workers share one
`whisper_context` and own one `whisper_state` each, like the in-game pool.

Build: generate with premake as usual (`--generator-version=3` for x64), then build the
`auris-bench` project, e.g. `MSBuild projects/windows/vs2022/auris-bench.vcxproj
-p:Configuration=Release -p:Platform=x64`.

```
auris-bench -m <model.bin> -w <file.wav|dir> [-j workers] [-t threads] [-r repeat]
            [-l lang] [--vad <ggml-silero.bin>] [--no-dynamic-ctx]
```

Input: 16 kHz mono WAV, PCM16 or float32. `-r` queues every clip that many times to load
the pool. Dynamic `audio_ctx` only applies to models whose filename contains `acft`.

Output per clip: audio length, wall ms, CPU-s per audio-s (process CPU delta, exact only
with `-j 1`), transcript. Summary: throughput, p50/p95 latency, total CPU-s per audio-s,
peak RSS. whisper.cpp has no fallback counter: outlier wall times are the sign of
temperature fallbacks or token loops.

Smoke test: `auris-bench -m ggml-tiny.en.bin -w voice.wav`.
