#!/bin/bash
set -e
D="$(cd "$(dirname "$0")" && pwd)"
P="$(cd "$D/.." && pwd)"
OUT="$P/output/庆元三_final"
mkdir -p "$OUT"

echo "=== QL3 PCA+Direction+ICP ==="
"$D/build/detect_pca" \
    "$P/source/庆元陈家岭隧道3.pcd" \
    "$P/output/targets_ql3/t1_noground.pcd" \
    "$P/output/targets_ql3/t2_noground.pcd" \
    "$OUT"

echo ""
cat "$OUT/results.csv"
ls -lh "$OUT"/match_*.pcd | head -5
