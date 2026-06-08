#!/bin/bash
set -e
D="$(cd "$(dirname "$0")" && pwd)"
P="$(cd "$D/.." && pwd)"
OUT="$P/output/庆元一_final"
mkdir -p "$OUT"

"$D/build/detect_pca" --voxel 0.08 \
    "$P/source/庆元陈家岭隧道1.pcd" \
    "$P/output/targets_ql3/t1_noground.pcd" \
    "$P/output/targets_ql3/t2_noground.pcd" \
    "$OUT" 2>&1 | grep -E "Voxel|Ground|Peel|Top 10|#0 f|#1 f|#2 f|#3 f|#4 f"
head -6 "$OUT/results.csv"
