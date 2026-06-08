#!/usr/bin/env python3
"""build_report.py — 生成 HTML 汇报页"""
import csv, os, glob, base64

datasets = [
    ('白峰岭', 'output/白峰岭_detect_v13'),
    ('庆元1', 'output/庆元1_detect_v13'),
    ('庆元2', 'output/庆元2_detect_v13'),
    ('庆元3', 'output/庆元3_detect_v13'),
    ('计量院', 'output/计量院_detect_v13'),
]

html = '''<!DOCTYPE html>
<html lang="zh">
<head><meta charset="utf-8"><title>标靶球检测报告</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif;background:#0d1117;color:#c9d1d9;padding:20px}
h1{text-align:center;color:#58a6ff;margin:20px 0 10px}
h2{color:#f0883e;border-bottom:1px solid #30363d;padding-bottom:8px;margin:30px 0 15px}
.pipeline{background:#161b22;border-radius:8px;padding:20px;max-width:900px;margin:0 auto 30px;font-size:14px;line-height:1.8}
.step{display:inline-block;background:#1f6feb;color:#fff;padding:2px 8px;border-radius:4px;margin:2px}
.arrow{color:#58a6ff}
table{width:100%;border-collapse:collapse;margin:10px 0 20px;font-size:13px}
th{background:#21262d;text-align:left;padding:8px 10px;border:1px solid #30363d}
td{padding:6px 10px;border:1px solid #30363d}
tr:hover{background:#161b22}
.gallery{display:flex;flex-wrap:wrap;gap:15px;justify-content:center;margin:15px 0}
.card{background:#161b22;border-radius:8px;overflow:hidden;width:380px}
.card img{width:100%;height:auto;display:block}
.card .info{padding:10px 12px;font-size:12px;line-height:1.5}
.tag{display:inline-block;padding:1px 6px;border-radius:3px;font-weight:bold;font-size:11px;margin-right:4px}
.tag-r{background:#da3633;color:#fff}.tag-c{background:#238636;color:#fff}
.summary{max-width:900px;margin:0 auto}
</style></head><body>
<h1>🎯 标靶球检测报告</h1>

<div class="pipeline">
<h3 style="color:#f0883e;margin-bottom:10px">管线逻辑</h3>
<p>
<span class="step">1. I>P80 初筛</span> <span class="arrow">→</span>
<span class="step">2. 3D 聚类</span> <span class="arrow">→</span>
<span class="step">3. 按模板取框</span> <span class="arrow">→</span>
<span class="step">4. SAC 平面 (只删地面 nz>0.7)</span> <span class="arrow">→</span>
<span class="step">5. k-means 双峰比 >5</span> <span class="arrow">→</span>
<span class="step">6. Z-crop 紧凑度 >0.9</span> <span class="arrow">→</span>
<span class="step">7. 输出</span>
</p>
<p style="color:#8b949e;margin-top:12px;font-size:13px">
<b>ratio</b> = hi_mean / lo_mean（双峰分离度，阈值 5）&nbsp;&nbsp;|&nbsp;&nbsp;
<b>compact</b> = max_cluster / crop_n（Z 窗内空间紧密度，阈值 0.9）&nbsp;&nbsp;|&nbsp;&nbsp;
<b>l2/l3</b> = PCA 特征值比（球壳薄度，参考值）&nbsp;&nbsp;|&nbsp;&nbsp;
<b>Z-crop</b> = hi点最高Z向下 1.2×球直径 (0.24m)
</p>
</div>

<div class="summary">
'''

for name, d in datasets:
    csv_path = os.path.join(d, 'results.csv')
    ss_dir = os.path.join(d, 'screenshots')
    if not os.path.exists(csv_path):
        continue

    html += f'<h2>{name}</h2>\n'
    html += '<table><tr><th>#</th><th>ratio</th><th>compact</th><th>l2/l3</th><th>X</th><th>Y</th><th>Z</th><th>hi_n</th><th>lo_n</th></tr>\n'

    rows = []
    with open(csv_path) as f:
        reader = csv.DictReader(f)
        for row in reader:
            rows.append(row)

    # Show top 5
    top5 = rows[:5]
    for row in top5:
        html += f'<tr><td>#{row["rank"]}</td><td><span class="tag tag-r">r={float(row["ratio"]):.1f}</span></td>'
        html += f'<td><span class="tag tag-c">c={float(row["compact"]):.2f}</span></td>'
        l2 = float(row.get('l2l3', 0))
        html += f'<td>{l2:.1f}</td>'
        html += f'<td>{float(row["cx"]):.2f}</td><td>{float(row["cy"]):.2f}</td><td>{float(row["cz"]):.2f}</td>'
        html += f'<td>{row["hi_n"]}</td><td>{row["lo_n"]}</td></tr>\n'
    html += '</table>\n'

    # Gallery of top 5 screenshots
    html += '<div class="gallery">\n'
    for row in top5:
        rank = int(row['rank'])
        ratio = float(row['ratio'])
        compact = float(row['compact'])
        cx, cy, cz = float(row['cx']), float(row['cy']), float(row['cz'])

        # Find matching screenshot
        ss_pattern = os.path.join(ss_dir, f'rank{rank:02d}_r{ratio:.0f}.png')
        matches = glob.glob(ss_pattern)
        if not matches:
            # Try nearby ratios
            matches = glob.glob(os.path.join(ss_dir, f'rank{rank:02d}_*.png'))
        if matches:
            with open(matches[0], 'rb') as imgf:
                b64 = base64.b64encode(imgf.read()).decode()
            html += f'<div class="card"><img src="data:image/png;base64,{b64}">'
            html += f'<div class="info"><b>#{rank}</b> '
            html += f'<span class="tag tag-r">ratio={ratio:.1f}</span> '
            html += f'<span class="tag tag-c">compact={compact:.2f}</span> '
            html += f'({cx:.2f}, {cy:.2f}, {cz:.2f})</div></div>\n'

    html += '</div>\n'

    # Also show remaining detections if any beyond top 5
    rest = rows[5:]
    if rest:
        html += f'<details><summary style="color:#8b949e;cursor:pointer;margin:10px 0">+ {len(rest)} more detections</summary>\n'
        html += '<table><tr><th>#</th><th>ratio</th><th>compact</th><th>X</th><th>Y</th><th>Z</th></tr>\n'
        for row in rest:
            html += f'<tr><td>#{row["rank"]}</td><td>{float(row["ratio"]):.1f}</td><td>{float(row["compact"]):.2f}</td>'
            html += f'<td>{float(row["cx"]):.2f}</td><td>{float(row["cy"]):.2f}</td><td>{float(row["cz"]):.2f}</td></tr>\n'
        html += '</table></details>\n'

html += '</div></body></html>'

with open('output/report.html', 'w') as f:
    f.write(html)
print('Saved output/report.html')
