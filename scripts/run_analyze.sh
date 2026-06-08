#!/bin/bash
set -e
D="$(cd "$(dirname "$0")" && pwd)"
cd "$D" && mkdir -p build
g++ -O2 -std=c++17 -o build/analyze_point analyze_point.cpp $(pkg-config --cflags pcl_common pcl_io pcl_filters pcl_segmentation pcl_search) $(pkg-config --libs pcl_common pcl_io pcl_filters pcl_segmentation pcl_search) 2>&1
cd "$(dirname "$D")"
exec "$D/build/analyze_point"
