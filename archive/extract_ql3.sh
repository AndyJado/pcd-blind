#!/bin/bash
set -e
D="$(cd "$(dirname "$0")" && pwd)"
P="$(cd "$D/.." && pwd)"
"$D/build/extract_target" \
    "$P/source/庆元陈家岭隧道3.pcd" \
    "$P/output/targets_ql3/t1_10_5.pcd" \
    9.0 10.6 4.3 5.9 -0.8 1.2
ls -lh "$P/output/targets_ql3/"
