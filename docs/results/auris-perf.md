# Auris performance at 128 players (lot I2)

Measured 2026-11-18 on the commit that adds this file. Host: Windows 11, 16 logical
CPUs, Linux runs in WSL2 Ubuntu (gcc 13), srcds lin64 dedicated server, 66 tick,
maxplayers 128, gm_construct.

## How it was measured

- **Load**: gm_realbots (lin64 only) spawns N bots that play real speech WAVs
  through `realbots.VoiceAudio` (Opus VOIP 24 kbps, 50 frames/s, same packets as
  a Steam client). Each talker speaks one clip, then stays quiet so it talks
  ~20 % of the time; starts are spread over one talk period. 128 talkers gave
  30 audio-s/s (1 793 s of voice per 60 s), the capacity target of the plan.
- **Scenario**: `tools/ingame` (`install.sh`, `run_case.sh`, `run_matrix.sh`,
  scenario Lua `auris_load`). 90 s measured window, 20 s tail for the last
  clips. Bots have no client Lua, so the scenario calls `auris.Flush` 250 ms
  after each clip, as `sv_auris_feed` does on the client's end-of-voice signal.
- **Latency** = flush → `Auris_Transcription` hook (queue wait + decode).
  **CPU-s/audio-s** = `auris.GetStats().cpu_per_audio` (worker threads only;
  partial with threads = 2). **Tick** = wall time between `Tick` hooks
  (p50/p99/max). **srcds CPU %** and **RSS** = whole process
  (`realbots.Process()`), so they include the bots.
- **Corpus** (`models/clips`, not committed): EN JFK inaugural + LibriSpeech,
  FR translation of the JFK line, cut at 1 / 3 / 8 s plus full clips, 16 kHz.
  References in `tools/ingame/refs.tsv`; `wer.py` scores each text against the
  closest reference of its language. 1 s cuts end mid-word (reported apart);
  `en_8s` / `fr_8s` repeat one sentence and are left out (whisper merges the
  repeat). WER is indicative: a few dozen distinct utterances, not a benchmark.
- **Caveat**: the 128 bots run inside srcds. Without Auris they already cost
  ~11 ms of the 15 ms tick on the main thread and push tick p99 to ~48 ms
  (`off_t128`). Compare Auris cells with the `off_t*` row of the same N, not
  with an idle server; a real 128-player server does not pay the bots.

## In-game matrix (lin64)

Cell names: model or engine, `tN` talkers, `wN` workers, `thN` threads per
worker (default 1). VAD (Silero) on and affinity on unless the name says
otherwise. `off_*` = Auris not installed.

| cell | talkers | results/flushes | dropped | lat p50 ms | lat p95 ms | CPU-s/audio-s | tick p50/p99/max ms | srcds CPU % | RSS MiB | WER % | CER % | WER 1 s % |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| off_t0 | 0 | - | - | - | - | - | 15.2/15.2/1249 | 7 | 198 | - | - | - |
| off_t16 | 16 | - | - | - | - | - | 15.2/16.1/17 | 18 | 239 | - | - | - |
| off_t64 | 64 | - | - | - | - | - | 15.2/32.0/195 | 48 | 593 | - | - | - |
| off_t128 | 128 | - | - | - | - | - | 15.1/48.3/356 | 88 | 1297 | - | - | - |
| tiny_en_acft_q8_0_t1_w4 | 1 | 3/3 | 0 | 182 | 955 | 0.084 | 15.2/15.3/17 | 8 | 498 | 17.9 | 20.8 | - |
| tiny_en_acft_q8_0_t16_w4 | 16 | 74/74 | 0 | 182 | 909 | 0.061 | 15.2/15.6/17 | 37 | 630 | 10.2 | 9.3 | 55.1 |
| tiny_en_acft_q8_0_t64_w4 | 64 | 308/309 | 0 | 182 | 941 | 0.064 | 15.1/41.2/167 | 126 | 964 | 9.6 | 8.6 | 51.0 |
| tiny_en_acft_q8_0_t128_w4 | 128 | 606/606 | 0 | 211 | 974 | 0.063 | 14.4/67.8/430 | 245 | 1653 | 9.2 | 8.3 | 51.2 |
| base_en_acft_q8_0_t1_w4 | 1 | 3/3 | 0 | 379 | 1561 | 0.142 | 15.2/15.3/17 | 9 | 605 | 21.4 | 28.3 | - |
| base_en_acft_q8_0_t16_w4 | 16 | 74/74 | 0 | 303 | 1424 | 0.108 | 15.2/15.6/17 | 52 | 779 | 12.3 | 11.1 | 62.3 |
| base_en_acft_q8_0_t64_w4 | 64 | 304/309 | 0 | 318 | 1742 | 0.122 | 15.2/33.3/154 | 198 | 1137 | 11.5 | 9.9 | 65.0 |
| base_en_acft_q8_0_t128_w4 | 128 | 584/606 | 22 | 534 | 9365 | 0.120 | 13.9/79.3/441 | 370 | 1717 | 13.1 | 11.6 | 84.8 |
| ggml-tiny.en_t1_w4 | 1 | 3/3 | 0 | 682 | 1955 | 0.220 | 15.2/15.3/17 | 10 | 613 | 17.9 | 20.8 | - |
| ggml-tiny.en_t16_w4 | 16 | 74/74 | 0 | 636 | 1910 | 0.196 | 15.1/15.6/17 | 79 | 763 | 10.2 | 9.3 | 26.1 |
| ggml-tiny.en_t64_w4 | 64 | 305/309 | 0 | 729 | 2500 | 0.214 | 15.2/36.2/110 | 315 | 1115 | 9.6 | 8.6 | 28.0 |
| ggml-tiny.en_t128_w4 | 128 | 427/606 | 179 | 2014 | 49783 | 0.226 | 13.2/86.9/246 | 471 | 1656 | 24.7 | 22.9 | 167.1 |
| base_en_acft_t128_w1_th1 | 128 | 212/606 | 394 | 25449 | 67800 | 0.122 | 13.9/73.9/441 | 189 | 1460 | 35.4 | 30.9 | 234.1 |
| base_en_acft_t128_w1_th2 | 128 | 357/606 | 249 | 7748 | 51367 | 0.068 | 13.9/68.8/304 | 244 | 1400 | 27.6 | 24.4 | 234.6 |
| base_en_acft_t128_w2_th1 | 128 | 399/606 | 207 | 2790 | 45695 | 0.118 | 13.8/71.3/702 | 274 | 1526 | 28.7 | 25.5 | 204.8 |
| base_en_acft_t128_w2_th2 | 128 | 542/606 | 64 | 733 | 25323 | 0.069 | 13.7/72.9/492 | 329 | 1547 | 18.6 | 16.6 | 126.3 |
| base_en_acft_t128_w4_th1 | 128 | 558/606 | 48 | 633 | 21891 | 0.129 | 12.8/103.2/550 | 385 | 1608 | 15.3 | 13.9 | 97.3 |
| base_en_acft_t128_w4_th2 | 128 | 603/606 | 3 | 334 | 1482 | 0.088 | 12.0/107.0/449 | 448 | 1535 | 11.9 | 10.7 | 59.8 |
| base_en_acft_t128_w4_novad | 128 | 574/606 | 32 | 594 | 19300 | 0.127 | 12.9/95.8/385 | 376 | 1629 | 16.0 | 14.2 | 85.9 |
| tiny_en_acft_t128_w4_noaff | 128 | 606/606 | 0 | 213 | 1022 | 0.065 | 14.3/64.3/690 | 254 | 1645 | 9.6 | 8.8 | 51.2 |
| base_acft_fr_t16_w4 | 16 | 68/68 | 0 | 333 | 500 | 0.072 | 15.2/15.6/18 | 41 | 701 | 10.2 | 11.8 | 89.1 |
| base_acft_fr_t128_w4 | 128 | 557/560 | 3 | 394 | 652 | 0.074 | 13.9/73.5/324 | 280 | 1736 | 11.0 | 12.7 | 89.7 |
| onnx_zipformer_t128_w4 | 128 | 606/606 | 0 | 106 | 201 | 0.012 | 14.4/65.1/452 | 121 | 1595 | 6.2 | 6.8 | 0.5 |
| onnx_moonshine_t128_w4 | 128 | 606/606 | 0 | 119 | 227 | 0.016 | 14.3/64.9/429 | 131 | 1380 | 6.3 | 6.3 | 170.3 |
| onnx_parakeet_en_t128_w4 | 128 | 511/606 | 14 | 1051 | 31619 | 0.113 | 13.3/85.0/381 | 368 | 2448 | 17.2 | 15.9 | 191.2 |
| onnx_parakeet_fr_t16_w4 | 16 | 68/68 | 0 | 409 | 955 | 0.107 | 15.2/15.6/17 | 51 | 1508 | 12.7 | 8.5 | 100.0 |
| onnx_parakeet_fr_t128_w4 | 128 | 539/560 | 21 | 711 | 18074 | 0.115 | 13.4/81.5/443 | 371 | 2550 | 16.2 | 12.8 | 126.0 |

### Main-thread cost per tick (ms, realbots meter)

| cell | main thread | whole process |
|---|---|---|
| off_t128 | 10.97 | 13.26 |
| tiny_en_acft_q8_0_t128_w4 | 11.45 | 37.07 |
| onnx_zipformer_t128_w4 | 11.51 | 18.29 |
| base_acft_fr_t128_w4 | 11.89 | 42.41 |
| base_en_acft_t128_w4_th2 | 12.97 | 68.49 |

Auris adds ~0.5 ms per tick on the main thread at 128 talkers (Opus decode in
the voice hook + result dispatch); more when the workers saturate the host.

## auris-bench, same corpus (sequential, 1 worker, 1 thread, VAD on)

All 12 clips (EN and FR together, 61.2 s), CPU-s per audio-s / latency p50 / p95.

| build | model | CPU-s/audio-s | p50 ms | p95 ms | RSS MiB |
|---|---|---|---|---|---|
| Win32 | tiny_en_acft_q8_0 | 0.070 | 218 | 728 | 113 |
| Win32 | base_en_acft_q8_0 | 0.140 | 529 | 1648 | 178 |
| Win32 | ggml-tiny.en (stock) | 0.641 | 2803 | 5314 | 163 |
| Win32 | base_acft_q8_0 -l fr | 0.138 | 467 | 1304 | 178 |
| Win x64 | tiny_en_acft_q8_0 | 0.033 | 120 | 335 | 140 |
| Win x64 | base_en_acft_q8_0 | 0.069 | 238 | 926 | 205 |
| Win x64 | ggml-tiny.en (stock) | 0.194 | 829 | 1630 | 190 |
| Win x64 | base_acft_q8_0 -l fr | 0.065 | 223 | 596 | 205 |
| Win x64 | onnx zipformer small EN | 0.025 | 43 | 94 | 226 |
| Win x64 | onnx moonshine tiny EN | 0.021 | 52 | 128 | 160 |
| Win x64 | onnx parakeet v3 int8 | 0.132 | 467 | 897 | 933 |
| lin32 | tiny_en_acft_q8_0 | 0.063 | 203 | 656 | 111 |
| lin64 | tiny_en_acft_q8_0 | 0.030 | 111 | 300 | 121 |

Windows has no in-game run: gm_realbots is lin64 only. GPU (Vulkan) builds
compile on Windows (`gmsv_auris-gpu_win64.dll`), but this host has no GPU and
auris-bench is CPU only, so they were not measured; no Vulkan SDK in WSL, so
the Linux GPU build was skipped.

## What the numbers say

- **ACFT is the biggest single win for whisper**: stock `ggml-tiny.en` costs
  3.5x the CPU of `tiny_en_acft_q8_0` (0.22 vs 0.063 CPU-s/audio-s in game) at
  the same WER, because only ACFT models may shrink `audio_ctx` to the clip.
  At 128 talkers the stock model drops 30 % of the clips with 4 workers.
- **Capacity rule**: workers must cover talk rate x CPU-s/audio-s with
  headroom. 128 players at 20 % talk time = 30 audio-s/s, so tiny ACFT needs
  ~2 cores (4 workers: 0 drops, p95 < 1 s), base ACFT EN ~4 cores (4 workers
  x 1 thread drops 8 %; 4 x 2 threads keeps up: 3 drops, p95 1.5 s), parakeet
  ~3.5 cores (4 workers drop a few %).
- **Workers beat threads per worker** for short clips at equal cores
  (w4 th1 > w2 th2 > w1 th2 for drops); `threads = 2` only helps once workers
  already match the core budget.
- **ONNX on 64-bit is the cheapest path for English**: zipformer small and
  moonshine tiny cost 0.012-0.016 CPU-s/audio-s at 128 talkers, p95 ~0.2 s,
  WER ~6 %, no drops with 4 workers (~0.5 core of work). Moonshine loops on the
  1 s cuts; zipformer does not (WER 1 s 0.5 %), but outputs upper case without
  punctuation.
- **French**: whisper `base_acft_q8_0` + `language = "fr"` keeps up at 128
  talkers with 4 workers (0.074 CPU-s/audio-s, p95 0.65 s, WER ~11 %); parakeet
  v3 is about as accurate on this corpus (WER 13-16 %, better punctuation) but
  costs 1.5x the CPU and 1.4x the RSS (2.5 GB process).
- **VAD**: no CPU difference on this corpus (every clip is speech, so the gate
  has nothing to crop); it pays off on real voice chat with open mics and
  breathing. Keep it on.
- **Affinity**: on vs off within noise here (tick p99 68 vs 64 ms). WSL exposes
  one L3, so only the "all but the main core" mask was exercised; the main
  core's SMT sibling is still in the worker mask (follow-up).

## Recommended 128-player defaults

| | 64-bit server | 32-bit server |
|---|---|---|
| English | `backend = "onnx"`, zipformer small (or moonshine tiny), workers 4, threads 1 | `tiny_en_acft_q8_0.bin`, workers 4, threads 1 |
| English, best whisper quality | `base_en_acft_q8_0.bin`, workers 4, threads 2 (8 spare cores) | not advised at 128p (2x slower than lin64) |
| French | `base_acft_q8_0.bin`, `language = "fr"`, workers 4, threads 1 | same, budget 2x the CPU |

Always: Silero VAD on, `affinity = true`, `max_queue_seconds` 30 (drops the
oldest clips rather than letting latency grow), never stock non-ACFT models.

## Bugs found and fixed during this lot

- One Opus decoder and sequence counter were shared by every speaker: two
  overlapping talkers decoded each other's frames. Now one decoder per speaker.
- Clients without a SteamID (sv_lan, bots) were all keyed 0 and could not be
  flushed from Lua (`AccountID()` = -1): keyed by entity index now
  (`Auris.VoiceKey`, 4th value of `auris.Poll`).
- Linux worker placement read the CPU set after pinning the main thread, so the
  worker mask was the game's own core. Uses the online CPU list now.
- The dropped-clips counter was shadowed and always reported 0.
- `auris.Flush` keys above 2^31 overflowed the int cast.

## Builds

| target | result |
|---|---|
| Win32 (v142) | gmsv_auris_win32.dll + auris-bench, no new source warnings |
| Win x64 | gmsv_auris_win64.dll links (first time) with `--gmcommon` pointing at an x86-64 capable garrysmod_common (gm_realbots' copy); the pinned submodule c29a9e8 still fails tier1 on x64 |
| Win x64 Vulkan | gmsv_auris-gpu_win64.dll builds; not run (no GPU on this host) |
| lin64 | gmsv_auris_linux64.dll with the same x86-64 garrysmod_common; tested in game |
| lin32 | gmsv_auris_linux.dll + auris-bench (multilib); bench only, realbots is lin64 only |
| Linux Vulkan | skipped: no Vulkan SDK in WSL |
