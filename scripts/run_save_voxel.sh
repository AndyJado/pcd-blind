#!/bin/bash
set -e
D="$(cd "$(dirname "$0")" && pwd)"
cd "$D" && mkdir -p build
g++ -O2 -std=c++17 -o build/save_voxel save_voxel.cpp \
    $(pkg-config --cflags pcl_common pcl_io pcl_filters) \
    $(pkg-config --libs pcl_common pcl_io pcl_filters) 2>&1
cd "$(dirname "$D")"
exec "$D/build/save_voxel"
