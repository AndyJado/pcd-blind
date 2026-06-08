#!/bin/bash
set -e
D="$(cd "$(dirname "$0")" && pwd)"
P="$(cd "$D/../.." && pwd)"
OUT="$P/output/ql1_2d"
mkdir -p "$OUT"

echo "=== Method 2D: QL1 ==="
"$D/build/detect_2d" \
    "$P/source/庆元陈家岭隧道1.pcd" \
    "$OUT" \
    2>&1 | grep -E "vox=|Ground |removed|Peel|Remain|Top|Done"

echo ""
echo "Top clusters:"
head -11 "$OUT/all_clusters.csv" 2>/dev/null
