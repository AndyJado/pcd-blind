#!/bin/bash
# regression_test.sh — 验证已知靶球全部检出
set -e

PROJ="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$PROJ/pcd-blind-rs/target/release/pcd-blind"
CONFIG="$PROJ/pcd-blind-rs/config/default.toml"
OUT="$PROJ/output/regression_test"

echo "=== Building ==="
cd "$PROJ/pcd-blind-rs" && ~/.cargo/bin/cargo build --release --quiet

rm -rf "$OUT"
PASS=0
FAIL=0

# 已知靶球坐标 (dataset -> expected targets)
declare -A TARGETS

TARGETS["庆元陈家岭隧道1"]="70.0,3.8;113.5,8.4;17.3,-2.8"
TARGETS["庆元陈家岭隧道2"]="40.3,-2.9;17.7,2.7;70.0,-1.0"
TARGETS["庆元陈家岭隧道3"]="58.9,9.6;9.8,5.2;53.5,14.4"
TARGETS["绍兴白峰岭隧道"]="21.6,8.1;32.8,8.1;33.2,-4.6;23.0,-1.9"

for scene in "${!TARGETS[@]}"; do
    echo ""
    echo "=== $scene ==="
    
    scene_out="$OUT/$scene"
    mkdir -p "$scene_out"
    
    $BIN -c "$CONFIG" "$PROJ/source/$scene.pcd" "$scene_out" 2>/dev/null
    
    IFS=';' read -ra COORDS <<< "${TARGETS[$scene]}"
    for target in "${COORDS[@]}"; do
        IFS=',' read -r tx ty <<< "$target"
        
        # Check if any detection is within 2m of the target
        FOUND=0
        while IFS=, read -r rank ratio compact tripod dxy gf score cx cy cz hi lo total; do
            [[ "$rank" == "rank" ]] && continue
            dx=$(echo "scale=3; $cx - $tx" | bc 2>/dev/null)
            dy=$(echo "scale=3; $cy - $ty" | bc 2>/dev/null)
            dx=${dx#-}; dy=${dy#-}
            dist=$(echo "scale=3; sqrt($dx*$dx + $dy*$dy)" | bc 2>/dev/null)
            if [[ -n "$dist" ]] && (( $(echo "$dist < 2.0" | bc -l) )); then
                FOUND=1
                echo "  ✅ ($tx,$ty) → rank$rank ($cx,$cy) d=${dist}m"
                break
            fi
        done < "$scene_out/results.csv"
        
        if [[ $FOUND -eq 0 ]]; then
            # Check candidates.csv too
            while IFS=, read -r idx ratio compact tripod dxy gf cx cy cz hi lo total; do
                [[ "$idx" == "idx" ]] && continue
                dx=$(echo "scale=3; $cx - $tx" | bc 2>/dev/null)
                dy=$(echo "scale=3; $cy - $ty" | bc 2>/dev/null)
                dx=${dx#-}; dy=${dy#-}
                dist=$(echo "scale=3; sqrt($dx*$dx + $dy*$dy)" | bc 2>/dev/null)
                if [[ -n "$dist" ]] && (( $(echo "$dist < 2.0" | bc -l) )); then
                    FOUND=1
                    echo "  ⚠️  ($tx,$ty) → candidates idx$idx ($cx,$cy) d=${dist}m (not in results)"
                    break
                fi
            done < "$scene_out/candidates.csv"
        fi
        
        if [[ $FOUND -eq 0 ]]; then
            echo "  ❌ ($tx,$ty) NOT FOUND"
            FAIL=$((FAIL + 1))
        else
            PASS=$((PASS + 1))
        fi
    done
done

echo ""
echo "=============================="
echo "  PASS: $PASS  FAIL: $FAIL"
echo "=============================="

if [[ $FAIL -gt 0 ]]; then
    exit 1
fi
