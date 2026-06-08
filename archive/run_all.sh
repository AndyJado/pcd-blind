#!/bin/bash
set -e
D="$(cd "$(dirname "$0")" && pwd)"
P="$(cd "$D/.." && pwd)"

for dataset in "庆元陈家岭隧道1" "庆元陈家岭隧道2" "浙江省计量院一楼实验室" "拓普康设备高精度点云"; do
    OUT="$P/output/${dataset}_final"
    mkdir -p "$OUT"
    echo ""
    echo "========================================="
    echo "  $dataset"
    echo "========================================="
    "$D/build/detect_pca" \
        "$P/source/${dataset}.pcd" \
        "$P/output/targets/t1_noground.pcd" \
        "$P/output/targets/t2_noground.pcd" \
        "$P/output/targets/t3_noground.pcd" \
        "$P/output/targets/t4_noground.pcd" \
        "$OUT" 2>&1 | grep -E "Voxel|Ground|Peel|Top 10|#0 f|#1 f|#2 f|#3 f|#4 f"
    echo ""
    head -6 "$OUT/results.csv" 2>/dev/null
done
