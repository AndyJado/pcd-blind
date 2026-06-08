#!/bin/bash
set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
OUTDIR="$PROJECT_DIR/output/绍兴白峰岭隧道_stripped"
mkdir -p "$OUTDIR"

# Extract template region from ORIGINAL full-res cloud
"$SCRIPT_DIR/build/extract_target" \
    "$PROJECT_DIR/source/绍兴白峰岭隧道.pcd" \
    "$OUTDIR/target_fullres.pcd" \
    32.5 33.5 7.7 8.5 -1.0 1.0

echo "Template (full res):"
"$SCRIPT_DIR/build/extract_target" /dev/null /dev/null 0 0 0 0 0 0 2>&1 | head -1
ls -lh "$OUTDIR"/target_fullres.pcd
