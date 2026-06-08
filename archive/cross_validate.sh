#!/bin/bash
set -e
D="$(cd "$(dirname "$0")" && pwd)"
P="$(cd "$D/.." && pwd)"
SCENE="$P/source/绍兴白峰岭隧道.pcd"
OUT="$P/output/cross_validate"
mkdir -p "$OUT"

declare -A TPLS
TPLS[t1]="$P/output/targets/t1_33_8.pcd"
TPLS[t2]="$P/output/targets/t2_19_-4.pcd"
TPLS[t3]="$P/output/targets/t3_33_-4.5.pcd"
TPLS[t4]="$P/output/targets/t4_22_8.pcd"

declare -A TX TY
TX[t1]="32.9"; TY[t1]="8.1"
TX[t2]="19.3"; TY[t2]="-4.0"
TX[t3]="33.1"; TY[t3]="-4.6"
TX[t4]="21.7"; TY[t4]="8.1"

echo "Cross-validation: can each template find the others?"
echo "==================================================="
echo ""

for Q in t1 t2 t3 t4; do
    echo ">>> Query: $Q (${TX[$Q]}, ${TY[$Q]})"
    TMPOUT="$OUT/$Q"
    rm -rf "$TMPOUT"; mkdir -p "$TMPOUT"
    "$D/build/match" "$SCENE" "${TPLS[$Q]}" "$TMPOUT" 2>&1 | grep "^#" 
    
    if [ -f "$TMPOUT/matches.csv" ]; then
        while IFS=, read -r id fit cx cy cz n; do
            [ "$id" = "id" ] && continue
            # check which target this match is closest to
            best="?"; best_dist=999
            for T in t1 t2 t3 t4; do
                dx=$(echo "$cx - ${TX[$T]}" | bc -l 2>/dev/null || echo 999)
                dy=$(echo "$cy - ${TY[$T]}" | bc -l 2>/dev/null || echo 999)
                dist=$(echo "sqrt($dx*$dx + $dy*$dy)" | bc -l 2>/dev/null || echo 999)
                if (( $(echo "$dist < $best_dist" | bc -l) )); then
                    best="$T"; best_dist=$dist
                fi
            done
            echo "  match: ($cx, $cy) fit=$fit → closest to $best (${best_dist}m)"
        done < "$TMPOUT/matches.csv"
    fi
    echo ""
done
