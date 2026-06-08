#!/bin/bash
set -e
D="$(cd "$(dirname "$0")" && pwd)"
P="$(cd "$D/.." && pwd)"
O="$P/output/fpfh_features/template"
mkdir -p "$O"
"$D/build/save_fpfh" "$P/output/绍兴白峰岭隧道_stripped/target_labeled.pcd" "$O"
echo "---"
ls -lh "$O/"
