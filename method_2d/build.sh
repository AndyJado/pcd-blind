#!/bin/bash
set -e
D="$(cd "$(dirname "$0")" && pwd)"
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
echo "Built: $D/build/detect_2d"
