#!/bin/bash
# regression_test.sh — 回归测试
# 测: 检出数 / 真靶覆盖率 / candidates覆盖率 / 性能
set -e

PROJ="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$PROJ/pcd-blind-rs/target/release/pcd-blind"
CONFIG="$PROJ/pcd-blind-rs/config/default.toml"
OUT="$PROJ/output/regression_test"
TIMEFILE="$OUT/timing.txt"

echo "=== Building ==="
cd "$PROJ/pcd-blind-rs" && ~/.cargo/bin/cargo build --release --quiet

rm -rf "$OUT"
mkdir -p "$OUT"

PASS=0; FAIL=0; WARN=0

# ── 数据集定义 ──

# 每个数据集： name | source_file | expected_min_detections | expected_max_detections
# 然后是已知靶球列表： cx,cy
declare -A META  # name -> "min max"
declare -A TARGETS  # name -> "cx,cy;cx,cy;..."

META["庆元陈家岭隧道1"]="3 8"
TARGETS["庆元陈家岭隧道1"]="70.0,3.8;113.5,8.4;17.3,-2.8"

META["庆元陈家岭隧道2"]="3 6"
TARGETS["庆元陈家岭隧道2"]="40.3,-2.9;17.7,2.7;70.0,-1.0"

META["庆元陈家岭隧道3"]="2 5"
TARGETS["庆元陈家岭隧道3"]="58.9,9.6;9.8,5.2;53.5,14.4"

META["绍兴白峰岭隧道"]="3 10"
TARGETS["绍兴白峰岭隧道"]="21.6,8.1;32.8,8.1;33.2,-4.6;23.0,-1.9"

# ── 辅助函数 ──
find_target() {
    local tx="$1" ty="$2" file="$3" label="$4" has_score="$5"
    local best_d=99 best_info=""
    if [[ "$has_score" == "1" ]]; then
        # results.csv: rank,ratio,compact,tripod,dxy,ground_frac,score,cx,cy,cz,...
        while IFS=, read -r rank ratio compact tripod dxy gf score cx cy cz rest; do
            [[ "$rank" == "rank" ]] && continue
            dx=$(awk "BEGIN{printf \"%.3f\", $cx - $tx}"); dx=${dx#-}
            dy=$(awk "BEGIN{printf \"%.3f\", $cy - $ty}"); dy=${dy#-}
            dist=$(awk "BEGIN{printf \"%.3f\", sqrt($dx*$dx + $dy*$dy)}")
            if awk "BEGIN{exit($dist < 2.0 ? 0 : 1)}"; then
                if awk "BEGIN{exit($dist < $best_d ? 0 : 1)}"; then
                    best_d=$dist
                    best_info="$label $rank tripod=$tripod ($cx,$cy) d=${dist}m"
                fi
            fi
        done < "$file"
    else
        # candidates.csv: idx,ratio,compact,tripod,dxy,ground_frac,cx,cy,cz,...
        while IFS=, read -r idx ratio compact tripod dxy gf cx cy cz rest; do
            [[ "$idx" == "idx" ]] && continue
            dx=$(awk "BEGIN{printf \"%.3f\", $cx - $tx}"); dx=${dx#-}
            dy=$(awk "BEGIN{printf \"%.3f\", $cy - $ty}"); dy=${dy#-}
            dist=$(awk "BEGIN{printf \"%.3f\", sqrt($dx*$dx + $dy*$dy)}")
            if awk "BEGIN{exit($dist < 2.0 ? 0 : 1)}"; then
                if awk "BEGIN{exit($dist < $best_d ? 0 : 1)}"; then
                    best_d=$dist
                    best_info="$label $idx tripod=$tripod ($cx,$cy) d=${dist}m"
                fi
            fi
        done < "$file"
    fi
    echo "$best_info"
}

# ── 逐数据集测试 ──

for scene in "${!META[@]}"; do
    echo ""
    echo "━━━ $scene ━━━"

    read min_det max_det <<< "${META[$scene]}"
    scene_out="$OUT/$scene"
    mkdir -p "$scene_out"

    # 计时
    START=$(date +%s)
    $BIN -c "$CONFIG" "$PROJ/source/$scene.pcd" "$scene_out" 2>/dev/null
    END=$(date +%s)
    ELAPSED=$((END - START))
    echo "$scene: ${ELAPSED}s" >> "$TIMEFILE"

    # ── 1. 检出数 ──
    n_results=$(tail -n +2 "$scene_out/results.csv" 2>/dev/null | wc -l | tr -d ' ')
    n_candidates=$(tail -n +2 "$scene_out/candidates.csv" 2>/dev/null | wc -l | tr -d ' ')

    if [[ $n_results -ge $min_det ]] && [[ $n_results -le $max_det ]]; then
        echo "  ✅ detections: $n_results (expected $min_det-$max_det)"
        PASS=$((PASS + 1))
    else
        echo "  ❌ detections: $n_results (expected $min_det-$max_det)"
        FAIL=$((FAIL + 1))
    fi

    echo "     candidates: $n_candidates"

    # ── 2. 真靶覆盖率 ──
    IFS=';' read -ra COORDS <<< "${TARGETS[$scene]}"
    for target in "${COORDS[@]}"; do
        IFS=',' read -r tx ty <<< "$target"

        in_results=$(find_target "$tx" "$ty" "$scene_out/results.csv" "results rank" 1)
        in_candidates=$(find_target "$tx" "$ty" "$scene_out/candidates.csv" "candidates idx" 0)

        if [[ -n "$in_results" ]]; then
            echo "  ✅ ($tx,$ty) → $in_results"
            PASS=$((PASS + 1))
        elif [[ -n "$in_candidates" ]]; then
            echo "  ⚠️  ($tx,$ty) → $in_candidates (NOT in results)"
            WARN=$((WARN + 1))
        else
            echo "  ❌ ($tx,$ty) NOT FOUND"
            FAIL=$((FAIL + 1))
        fi
    done
done

# ── 3. 性能回归 ──
echo ""
echo "━━━ Performance ━━━"
while IFS=: read -r scene secs; do
    echo "  $scene: ${secs}s"
done < "$TIMEFILE"

# ── 结果 ──
echo ""
echo "=============================="
printf "  PASS: %2d  WARN: %2d  FAIL: %2d\n" $PASS $WARN $FAIL
echo "=============================="

if [[ $FAIL -gt 0 ]]; then
    exit 1
fi
[[ $WARN -gt 0 ]] && echo "⚠️  Some targets only in candidates (not results) — check gates"
