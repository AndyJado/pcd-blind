#!/bin/bash
set -e
D="$(cd "$(dirname "$0")" && pwd)"
P="$(cd "$D/.." && pwd)"
SRC="$P/source/浙江省计量院一楼实验室.pcd"
OUT="$P/output/targets_lab"
mkdir -p "$OUT"
EX="$D/build/extract_target"

# T2: wider box
"$EX" "$SRC" "$OUT/t2_53_14.pcd"  52.0 55.0 13.0 16.0 -1.0 2.0
echo "T2 wide: $(ls -lh "$OUT/t2_53_14.pcd" | awk '{print $5}')"

# even wider
"$EX" "$SRC" "$OUT/t2_wide.pcd"  51.0 56.0 12.0 17.0 -1.5 2.5
echo "T2 wider: $(ls -lh "$OUT/t2_wide.pcd" | awk '{print $5}')"
