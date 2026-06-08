#!/bin/bash
# run_peel_pipe.sh — peel tunnel shell → PCA filter → ICP rank
set -e
D="$(cd "$(dirname "$0")" && pwd)"
P="$(cd "$D/.." && pwd)"
OUT="$P/output/白峰岭_peel_icp"
mkdir -p "$OUT"

# Step 1: peel tunnel
echo "=== Peeling tunnel shell ==="
"$D/build/peel_tunnel"

# Step 2: PCA + ICP on peeled cloud
echo ""
echo "=== PCA + ICP pipeline ==="
"$D/build/detect_pca" \
    "$P/output/tunnel_peeled.pcd" \
    "$P/output/targets/t1_33_8.pcd" \
    "$P/output/targets/t2_19_-4.pcd" \
    "$P/output/targets/t3_33_-4.5.pcd" \
    "$P/output/targets/t4_22_8.pcd" \
    "$OUT"

echo ""
echo "Results:"
cat "$OUT/results.csv" 2>/dev/null
ls -lh "$OUT"/match_*.pcd 2>/dev/null | head -10
