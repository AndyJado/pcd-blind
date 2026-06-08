#!/bin/bash
set -e
D="$(cd "$(dirname "$0")" && pwd)"
P="$(cd "$D/.." && pwd)"
OUT="$P/output/白峰岭_pca_icp"
mkdir -p "$OUT"

echo "=== PCA + ICP pipeline ==="
"$D/build/detect_pca" \
    "$P/source/绍兴白峰岭隧道.pcd" \
    "$P/output/targets/t1_33_8.pcd" \
    "$P/output/targets/t2_19_-4.pcd" \
    "$P/output/targets/t3_33_-4.5.pcd" \
    "$P/output/targets/t4_22_8.pcd" \
    "$OUT"

echo ""
echo "Results:"
ls -lh "$OUT"/match_*.pcd 2>/dev/null | head -10
cat "$OUT/results.csv" 2>/dev/null
