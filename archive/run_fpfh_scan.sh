#!/bin/bash
set -e
D="$(cd "$(dirname "$0")" && pwd)"
P="$(cd "$D/.." && pwd)"
"$D/build/fpfh_scan" "$P/output/绍兴白峰岭隧道_stripped/target_labeled.pcd"
