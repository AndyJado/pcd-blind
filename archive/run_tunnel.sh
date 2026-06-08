#!/bin/bash
set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
OUTDIR="$PROJECT_DIR/output/绍兴白峰岭隧道_template"
mkdir -p "$OUTDIR"

echo "=== Template matching ==="
"$SCRIPT_DIR/build/detect_spheres" \
    "$PROJECT_DIR/source/绍兴白峰岭隧道.pcd" \
    "$PROJECT_DIR/output/绍兴白峰岭隧道_stripped/target_labeled.pcd" \
    "$OUTDIR"

echo ""
echo "Output:"
ls -lh "$OUTDIR"/match_*.pcd | head -10
echo "..."
ls "$OUTDIR"/match_*.pcd | wc -l
echo "match files"
