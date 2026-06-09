#!/usr/bin/env python3
"""dedup.py — 合并同XY位置的重复检测, 保留compact最高的"""
import csv, sys, math

def cluster(rows, tol=0.5):
    clusters = []
    for r in rows:
        cx, cy = float(r['cx']), float(r['cy'])
        found = False
        for c in clusters:
            mx = sum(float(r2['cx']) for r2 in c) / len(c)
            my = sum(float(r2['cy']) for r2 in c) / len(c)
            if math.sqrt((cx-mx)**2 + (cy-my)**2) < tol:
                c.append(r)
                found = True
                break
        if not found:
            clusters.append([r])
    # Keep best compact per cluster
    return [max(c, key=lambda r: float(r['compact'])) for c in clusters]

for path in sys.argv[1:]:
    with open(path) as f:
        rows = list(csv.DictReader(f))
    merged = cluster(rows)
    merged.sort(key=lambda r: -float(r['compact']))
    out = path.replace('.csv', '_dedup.csv')
    with open(out, 'w', newline='') as f:
        w = csv.DictWriter(f, fieldnames=rows[0].keys())
        w.writeheader()
        for i, r in enumerate(merged):
            r['rank'] = str(i)
            w.writerow(r)
    print(f"{path}: {len(rows)} → {len(merged)} → {out}")
