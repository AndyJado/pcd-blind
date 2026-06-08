#!/bin/bash
# Test: can SAC-IA match template to itself?
set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
TPL="$PROJECT_DIR/output/绍兴白峰岭隧道_stripped/target_labeled.pcd"

# Run match with scene = template itself
"$SCRIPT_DIR/build/match" "$TPL" "$TPL" /tmp/selftest

echo "=== Self-match result ==="
cat /tmp/selftest/matches.csv
