#!/bin/bash
set -e
D="$(cd "$(dirname "$0")" && pwd)"
P="$(cd "$D/.." && pwd)"
"$D/build/strip_ground_single" "$P/output/targets_ql3/t1_10_5_v2.pcd" "$P/output/targets_ql3/t1_noground.pcd"
"$D/build/strip_ground_single" "$P/output/targets_ql3/t2_53_14.pcd" "$P/output/targets_ql3/t2_noground.pcd"
ls -lh "$P/output/targets_ql3/"*noground*
