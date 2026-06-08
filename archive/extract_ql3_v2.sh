#!/bin/bash
set -e
D="$(cd "$(dirname "$0")" && pwd)"
P="$(cd "$D/.." && pwd)"
OUT="$P/output/targets_ql3"
mkdir -p "$OUT"

# Tighter box around pick (9.81, 5.07, 0.45): sphere ~0.6m above ground, tripod ~1.2m
"$D/build/extract_target" \
    "$P/source/庆元陈家岭隧道3.pcd" \
    "$OUT/t1_10_5_v2.pcd" \
    9.3 10.3 4.6 5.5 -1.2 0.7
ls -lh "$OUT/"
