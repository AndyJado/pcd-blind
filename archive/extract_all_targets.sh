#!/bin/bash
set -e
D="$(cd "$(dirname "$0")" && pwd)"
P="$(cd "$D/.." && pwd)"
SRC="$P/source/绍兴白峰岭隧道.pcd"
OUT="$P/output/targets"
mkdir -p "$OUT"
EX="$D/build/extract_target"

# T1: already exists, copy
cp "$P/output/绍兴白峰岭隧道_stripped/target_labeled.pcd" "$OUT/t1_33_8.pcd"

# T2: (19.29, -4.04, 0.21) — box ±0.8m X, ±0.7m Y, -0.8 to +1.2 Z
echo "Extracting T2..."
"$EX" "$SRC" "$OUT/t2_19_-4.pcd" \
    18.5 20.1 -4.7 -3.3 -0.8 1.2

# T3: (33.08, -4.59, 0.48) — box ±0.8m X, ±0.7m Y, -0.8 to +1.2 Z
echo "Extracting T3..."
"$EX" "$SRC" "$OUT/t3_33_-4.5.pcd" \
    32.3 33.9 -5.3 -3.9 -0.8 1.2

echo ""
echo "Targets:"
ls -lh "$OUT"/
