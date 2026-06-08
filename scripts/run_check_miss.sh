#!/bin/bash
set -e
D="$(cd "$(dirname "$0")" && pwd)"
cd "$D" && mkdir -p build
g++ -O2 -std=c++17 -o build/check_miss check_miss.cpp \
    $(pkg-config --cflags pcl_common pcl_io) \
    $(pkg-config --libs pcl_common pcl_io) 2>&1
cd "$(dirname "$D")"
exec "$D/build/check_miss"
