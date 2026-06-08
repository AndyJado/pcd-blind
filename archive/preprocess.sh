#!/bin/bash
# preprocess only: ground removal + peel → save for inspection
set -e
D="$(cd "$(dirname "$0")" && pwd)"
P="$(cd "$D/.." && pwd)"
"$D/build/preprocess"
echo "Saved: $P/output/preprocessed.pcd"
