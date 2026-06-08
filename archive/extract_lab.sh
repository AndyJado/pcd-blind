#!/bin/bash
set -e
D="$(cd "$(dirname "$0")" && pwd)"
P="$(cd "$D/.." && pwd)"
SRC="$P/source/浙江省计量院一楼实验室.pcd"
OUT="$P/output/targets_lab"
mkdir -p "$OUT"
EX="$D/build/extract_target"

"$EX" "$SRC" "$OUT/t1_10_5.pcd"   9.0 10.5  4.2 5.8  -0.8 1.2
echo "T1: $(ls -lh "$OUT/t1_10_5.pcd" | awk '{print $5}')"

"$EX" "$SRC" "$OUT/t2_53_14.pcd"  52.5 54.3 13.5 15.3 -0.3 1.7
echo "T2: $(ls -lh "$OUT/t2_53_14.pcd" | awk '{print $5}')"
