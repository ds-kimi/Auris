#!/usr/bin/env bash
# Lot I2 matrix on lin64 (WSL): one auris_load run per row, results in $OUT.
# Each row: label talkers clips [config key=value ...]. A reduced matrix: the
# workers x threads and VAD sweeps run at 128 talkers only, where they matter.
set -uo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
export OUT="${OUT:-$HOME/auris_i2/results}"
# 90 s: the first ~20 s ramp up while the staggered talkers start
export DURATION="${DURATION:-90}"
EN="en_1s,en_3s,en_8s,en_full,en2_1s,en2_3s,en2_8s"
FR="fr_1s,fr_3s,fr_8s,fr_full"
M='garrysmod/data/auris'
run() { bash "$here/run_case.sh" "$@"; }

bash "$here/install.sh" --no-auris > /dev/null
for n in 0 16 64 128; do run "off_t$n" "$n" "$EN"; done

bash "$here/install.sh" > /dev/null
for model in tiny_en_acft_q8_0 base_en_acft_q8_0 ggml-tiny.en; do
    for n in 1 16 64 128; do run "${model}_t${n}_w4" "$n" "$EN" model="\"$M/$model.bin\"" workers=4; done
done
for w in 1 2 4; do
    for t in 1 2; do run "base_en_acft_t128_w${w}_th${t}" 128 "$EN" model="\"$M/base_en_acft_q8_0.bin\"" workers=$w threads=$t; done
done
run "base_en_acft_t128_w4_novad" 128 "$EN" model="\"$M/base_en_acft_q8_0.bin\"" workers=4 vad_model='""'
run "tiny_en_acft_t128_w4_noaff" 128 "$EN" model="\"$M/tiny_en_acft_q8_0.bin\"" workers=4 affinity=false
for n in 16 128; do run "base_acft_fr_t${n}_w4" "$n" "$FR" model="\"$M/base_acft_q8_0.bin\"" language='"fr"' workers=4; done
O="$M/sherpa-onnx"
run onnx_zipformer_t128_w4 128 "$EN" backend='"onnx"' onnx_model_type='"zipformer"' onnx_model_dir="\"$O-zipformer-small-en-2023-06-26\"" workers=4
run onnx_moonshine_t128_w4 128 "$EN" backend='"onnx"' onnx_model_type='"moonshine"' onnx_model_dir="\"$O-moonshine-tiny-en-quantized-2026-02-27\"" workers=4
run onnx_parakeet_en_t128_w4 128 "$EN" backend='"onnx"' onnx_model_type='"parakeet"' onnx_model_dir="\"$O-nemo-parakeet-tdt-0.6b-v3-int8\"" workers=4
run onnx_parakeet_fr_t128_w4 128 "$FR" backend='"onnx"' onnx_model_type='"parakeet"' onnx_model_dir="\"$O-nemo-parakeet-tdt-0.6b-v3-int8\"" workers=4 language='"fr"'
run onnx_parakeet_fr_t16_w4 16 "$FR" backend='"onnx"' onnx_model_type='"parakeet"' onnx_model_dir="\"$O-nemo-parakeet-tdt-0.6b-v3-int8\"" workers=4 language='"fr"'
echo matrix done
