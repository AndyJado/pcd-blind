#!/bin/bash
set -e
D="$(cd "$(dirname "$0")" && pwd)"
P="$(cd "$D/.." && pwd)"
OUT="$P/output/庆元一_final"
mkdir -p "$OUT"

echo "=== QL1 ==="
"$D/build/detect_pca" \
    "$P/source/庆元陈家岭隧道1.pcd" \
    "$P/output/targets_ql3/t1_noground.pcd" \
    "$P/output/targets_ql3/t2_noground.pcd" \
    "$OUT" 2>&1 | grep -E "vox=|Ground|removed|Peel|Remain|Top|#0 f|#1 f|#2 f"
head -6 "$OUT/results.csv"
