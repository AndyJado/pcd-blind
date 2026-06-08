#!/bin/bash
set -e
D="$(cd "$(dirname "$0")" && pwd)"
PROJ="$(dirname "$D")"

echo "=== Building detect_targets ==="
cd "$D"
mkdir -p build
g++ -O2 -std=c++17 -o build/detect_targets detect_targets.cpp \
    $(pkg-config --cflags pcl_common pcl_io pcl_filters pcl_segmentation pcl_search pcl_sample_consensus) \
    $(pkg-config --libs pcl_common pcl_io pcl_filters pcl_segmentation pcl_search pcl_sample_consensus) \
    2>&1

echo "=== Running ==="
cd "$PROJ"
exec "$D/build/detect_targets" "$@"
