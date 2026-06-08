#!/bin/bash
set -e
D="$(cd "$(dirname "$0")" && pwd)"
P="$(cd "$D/.." && pwd)"
OUT="$P/output/targets_ql3"
mkdir -p "$OUT"

# T2 from QL3: (53.38, 14.40, 1.08)
"$D/build/extract_target" \
    "$P/source/庆元陈家岭隧道3.pcd" \
    "$OUT/t2_53_14.pcd" \
    52.8 53.9 13.9 15.0 -1.2 1.5
ls -lh "$OUT/t2_53_14.pcd"
