#!/bin/bash
set -e
D="$(cd "$(dirname "$0")" && pwd)"
P="$(cd "$D/.." && pwd)"
"$D/build/extract_target" \
    "$P/source/绍兴白峰岭隧道.pcd" \
    "$P/output/targets/t4_22_8.pcd" \
    20.9 22.5 7.3 8.9 -0.8 1.2
echo "done"
