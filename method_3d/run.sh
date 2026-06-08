#!/bin/bash
set -e
D="$(cd "$(dirname "$0")" && pwd)"
P="$(cd "$D/.." && pwd)"
OUT="$P/output/白峰岭_final"
mkdir -p "$OUT"

echo "=== PCA+Direction+ICP ==="
"$D/build/detect_pca" \
    "$P/source/绍兴白峰岭隧道.pcd" \
    "$P/output/targets/t1_noground.pcd" \
    "$P/output/targets/t2_noground.pcd" \
    "$P/output/targets/t3_noground.pcd" \
    "$P/output/targets/t4_noground.pcd" \
    "$OUT"

echo ""
echo "Results:"
cat "$OUT/results.csv"
ls -lh "$OUT"/match_*.pcd | head -10
