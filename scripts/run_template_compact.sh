#!/bin/bash
set -e
D="$(cd "$(dirname "$0")" && pwd)"
cd "$D" && mkdir -p build
g++ -O2 -std=c++17 -o build/template_compact template_compact.cpp \
    $(pkg-config --cflags pcl_common pcl_io pcl_segmentation pcl_search) \
    $(pkg-config --libs pcl_common pcl_io pcl_segmentation pcl_search) 2>&1
cd "$(dirname "$D")"
exec "$D/build/template_compact"
