#!/usr/bin/env python3
"""build_ppt.py v3 — 白底, 大图聚焦, 少文字"""
import csv, os, glob
from pptx import Presentation
from pptx.util import Inches, Pt, Emu
from pptx.dml.color import RGBColor
from pptx.enum.text import PP_ALIGN

BG  = RGBColor(0xFF, 0xFF, 0xFF)
DK  = RGBColor(0x1A, 0x1A, 0x2E)
ACC = RGBColor(0x1F, 0x6F, 0xEB)
ORG = RGBColor(0xE3, 0x6A, 0x1B)
GRN = RGBColor(0x1A, 0x8C, 0x3E)
GRY = RGBColor(0x66, 0x66, 0x77)
WHT = RGBColor(0xFF, 0xFF, 0xFF)
MID = RGBColor(0x33, 0x33, 0x44)

datasets = [
    ('白峰岭隧道', 'output/白峰岭_detect_v13', 'source/绍兴白峰岭隧道.pcd'),
    ('庆元陈家岭隧道1', 'output/庆元1_detect_v13', 'source/庆元陈家岭隧道1.pcd'),
    ('庆元陈家岭隧道2', 'output/庆元2_detect_v13', 'source/庆元陈家岭隧道2.pcd'),
    ('庆元陈家岭隧道3', 'output/庆元3_detect_v13', 'source/庆元陈家岭隧道3.pcd'),
    ('计量院实验室', 'output/计量院_detect_v13', 'source/浙江省计量院一楼实验室.pcd'),
]

prs = Presentation()
prs.slide_width  = Inches(13.333)
prs.slide_height = Inches(7.5)

def set_bg(slide, color=BG):
    slide.background.fill.solid()
    slide.background.fill.fore_color.rgb = color

def txt(slide, text, left, top, width, height, size=18, color=DK, bold=False, align=PP_ALIGN.LEFT):
    txBox = slide.shapes.add_textbox(Inches(left), Inches(top), Inches(width), Inches(height))
    tf = txBox.text_frame; tf.word_wrap = True
    p = tf.paragraphs[0]; p.text = text
    p.font.size = Pt(size); p.font.color.rgb = color; p.font.bold = bold; p.alignment = align
    return txBox

def img(slide, path, left, top, width, height=None):
    if os.path.exists(path):
        kw = {'left': Inches(left), 'top': Inches(top), 'width': Inches(width)}
        if height: kw['height'] = Inches(height)
        slide.shapes.add_picture(path, **kw)

def count_pcd_pts(path):
    try:
        with open(path, 'rb') as f:
            for _ in range(20):
                line = f.readline().decode('utf-8', errors='replace').strip()
                if line.startswith('POINTS'): return int(line.split()[1])
    except: pass
    return 0

# ===== SLIDE 1: 封面 =====
s = prs.slides.add_slide(prs.slide_layouts[6]); set_bg(s)
txt(s, "标靶球自动检测", 1.5, 2.0, 10, 1.2, size=48, color=DK, bold=True)
txt(s, "基于强度双峰分离 + 空间紧凑度", 1.5, 3.2, 10, 0.6, size=20, color=ORG)
txt(s, "5个隧道数据集 · 40个候选 · 白峰岭4/4全中", 1.5, 4.0, 10, 0.5, size=16, color=GRY)
line = s.shapes.add_shape(1, Inches(1.5), Inches(4.8), Inches(3), Inches(0.03))
line.fill.solid(); line.fill.fore_color.rgb = ACC

# ===== SLIDE 2: 管线 + 筛选漏斗 =====
s = prs.slides.add_slide(prs.slide_layouts[6]); set_bg(s)
txt(s, "检测管线", 0.5, 0.3, 5, 0.6, size=28, color=DK, bold=True)
steps = [
    ("① I>P80", "全局强度初筛"),
    ("② 3D聚类", "空间分组"),
    ("③ 取框", "1.2×1.2×2.3m"),
    ("④ SAC", "删地面留墙"),
    ("⑤ k-means", "双峰比>5"),
    ("⑥ Z-crop", "紧凑度>0.9"),
    ("⑦ 输出", "CSV + 3D图"),
]
y = 1.2
for i, (title, desc) in enumerate(steps):
    color = ACC if i < 6 else GRN
    txt(s, title, 0.8, y, 1.8, 0.35, size=14, color=color, bold=True)
    txt(s, desc, 2.8, y, 2.5, 0.35, size=12, color=GRY)
    # arrow
    if i < len(steps)-1:
        txt(s, "→", 2.3, y, 0.5, 0.35, size=14, color=GRY)
    y += 0.48

# Funnel on right side
txt(s, "筛选漏斗 (白峰岭)", 7, 1.2, 5, 0.4, size=16, color=DK, bold=True)
funnel = [
    ("3,487,129 pt", "原始点云", 10),
    ("795,078", "I>P80=29", 8.5),
    ("112 clusters", "3D分组", 7),
    ("~80", "有平面", 5),
    ("9", "ratio+compact", 3),
]
y = 1.8
for pts, label, w in funnel:
    txt(s, pts, 7.0, y, 2, 0.3, size=12, color=DK, bold=True)
    bar = s.shapes.add_shape(1, Inches(9.5), Inches(y+0.05), Inches(w*0.3), Inches(0.2))
    bar.fill.solid();
    if '9' in pts: bar.fill.fore_color.rgb = GRN
    elif '80' in pts: bar.fill.fore_color.rgb = ORG
    else: bar.fill.fore_color.rgb = ACC
    txt(s, label, 7.0, y+0.3, 5, 0.25, size=9, color=GRY)
    y += 0.65

# ===== SLIDE 3: 数据总览 (单页表格) =====
s = prs.slides.add_slide(prs.slide_layouts[6]); set_bg(s)
txt(s, "数据总览", 0.5, 0.3, 5, 0.6, size=28, color=DK, bold=True)

# Build table
rows = [['数据集', '原始点数', 'I>P80', 'I阈值', '聚类数', '检出', '已知']]
total_map = {}
for dname, out_d, src in datasets:
    total = count_pcd_pts(src); total_map[dname] = total
    hi_n = int(total * 0.2)
    with open(os.path.join(out_d, 'results.csv')) as f:
        n_det = len(list(csv.DictReader(f)))
    known = '4/4 ✓' if '白峰岭' in dname else '—'
    rows.append([dname, f'{total/1e6:.1f}M', f'{hi_n/1e6:.1f}M', '29–42', '54–112', str(n_det), known])

n_rows, n_cols = len(rows), len(rows[0])
ts = s.shapes.add_table(n_rows, n_cols, Inches(0.8), Inches(1.3), Inches(11.5), Inches(3.5))
t = ts.table
col_w = [3.0, 1.5, 1.5, 1.0, 1.2, 1.0, 1.3]
for i, w in enumerate(col_w): t.columns[i].width = Inches(w)
for r, row in enumerate(rows):
    for c, val in enumerate(row):
        cell = t.cell(r, c); cell.text = str(val)
        for p in cell.text_frame.paragraphs:
            p.font.size = Pt(12) if r==0 else Pt(11)
            p.font.color.rgb = WHT if r==0 else DK
            p.font.bold = (r==0)
        if r == 0:
            cell.fill.solid(); cell.fill.fore_color.rgb = ACC
        else:
            cell.fill.solid(); cell.fill.fore_color.rgb = RGBColor(0xF0, 0xF2, 0xF5)

txt(s, "白峰岭 4/4 已知标靶全部检出 | 其余数据集候选待现场验证", 0.8, 5.2, 11, 0.4, size=13, color=GRY)

# ===== SLIDES 4-13: 每个数据集2页 =====
for dname, out_d, src in datasets:
    csv_path = os.path.join(out_d, 'results.csv')
    ss_dir = os.path.join(out_d, 'screenshots')
    ov_path = os.path.join(out_d, 'overview.png')
    if not os.path.exists(csv_path): continue

    with open(csv_path) as f:
        rows = list(csv.DictReader(f))

    # --- Page A: Overview + top detection info ---
    s = prs.slides.add_slide(prs.slide_layouts[6]); set_bg(s)
    txt(s, dname, 0.5, 0.3, 7, 0.6, size=28, color=DK, bold=True)
    total = total_map.get(dname, 0)
    top = rows[0] if rows else None
    if top:
        txt(s, f"检出 {len(rows)} 个候选 | 最高 ratio={float(top['ratio']):.1f} | 原始 {total/1e6:.1f}M pt",
             0.5, 0.85, 8, 0.35, size=13, color=GRY)

    # Overview image — large
    if os.path.exists(ov_path):
        img(s, ov_path, 0.5, 1.4, 8.5)

    # Top 3 info cards on the right
    xc, yc = 9.5, 1.4
    for i in range(min(3, len(rows))):
        row = rows[i]
        rank, ratio, compact = int(row['rank']), float(row['ratio']), float(row['compact'])
        cx, cy, cz = float(row['cx']), float(row['cy']), float(row['cz'])
        # Card background
        card = s.shapes.add_shape(1, Inches(xc), Inches(yc), Inches(3.3), Inches(1.5))
        card.fill.solid(); card.fill.fore_color.rgb = RGBColor(0xF0, 0xF2, 0xF5)
        card.line.fill.background()
        txt(s, f"#{rank}", xc+0.15, yc+0.1, 0.5, 0.3, size=16, color=ACC, bold=True)
        txt(s, f"ratio {ratio:.1f}", xc+0.7, yc+0.1, 1.5, 0.3, size=14, color=DK, bold=True)
        txt(s, f"({cx:.2f}, {cy:.2f}, {cz:.2f})", xc+0.15, yc+0.45, 3, 0.3, size=10, color=GRY)
        txt(s, f"compact {compact:.2f}  |  hi {row['hi_n']}  |  lo {row['lo_n']}", xc+0.15, yc+0.75, 3, 0.3, size=10, color=GRY)
        if 'l2l3' in row:
            txt(s, f"l2/l3 {float(row['l2l3']):.1f}", xc+0.15, yc+1.0, 3, 0.3, size=10, color=GRY)
        yc += 1.7

    # --- Page B: 3D screenshots (large) ---
    s = prs.slides.add_slide(prs.slide_layouts[6]); set_bg(s)
    txt(s, f"{dname} — Top 检测 3D 视图", 0.5, 0.3, 8, 0.6, size=24, color=DK, bold=True)

    # Show detection screenshots in a 2×3 or 1×3 grid
    n_show = min(4, len(rows))
    if n_show <= 2:
        # 2 large images
        img_w, img_h = 5.8, 4.3
        for i in range(n_show):
            row = rows[i]; rank = int(row['rank']); ratio = float(row['ratio'])
            ss_p = os.path.join(ss_dir, f'rank{rank:02d}_r{ratio:.0f}.png')
            if not os.path.exists(ss_p):
                matches = glob.glob(os.path.join(ss_dir, f'rank{rank:02d}_*.png'))
                ss_p = matches[0] if matches else ''
            if ss_p and os.path.exists(ss_p):
                img(s, ss_p, 0.5 + i*6.5, 1.2, img_w, img_h)
                txt(s, f"#{rank}  ratio={ratio:.1f}  compact={float(row['compact']):.2f}",
                    0.5 + i*6.5, 1.2+img_h+0.05, img_w, 0.3, size=12, color=DK, bold=True, align=PP_ALIGN.CENTER)
    else:
        # 2×2 grid
        x_offs = [0.5, 6.5]; y_offs = [1.2, 4.3]
        img_w, img_h = 5.5, 2.8
        for i in range(n_show):
            row = rows[i]; rank = int(row['rank']); ratio = float(row['ratio'])
            ss_p = os.path.join(ss_dir, f'rank{rank:02d}_r{ratio:.0f}.png')
            if not os.path.exists(ss_p):
                matches2 = glob.glob(os.path.join(ss_dir, f'rank{rank:02d}_*.png'))
                ss_p = matches2[0] if matches2 else ''
            xi, yi = i % 2, i // 2
            if ss_p and os.path.exists(ss_p):
                img(s, ss_p, x_offs[xi], y_offs[yi], img_w, img_h)
                txt(s, f"#{rank}  r={ratio:.1f}  c={float(row['compact']):.2f}  ({float(row['cx']):.1f},{float(row['cy']):.1f},{float(row['cz']):.1f})",
                    x_offs[xi], y_offs[yi]+img_h+0.05, img_w, 0.25, size=10, color=DK, align=PP_ALIGN.CENTER)

# ===== LAST SLIDE: 总结 =====
s = prs.slides.add_slide(prs.slide_layouts[6]); set_bg(s)
txt(s, "总结", 0.5, 0.3, 5, 0.6, size=28, color=DK, bold=True)

sections = [
    ("成果", ACC, [
        "白峰岭4/4已知标靶全部检出 (ratio 6.1–9.1, compact≥0.97)",
        "5个数据集共检出40个候选，筛选效率高",
        "强度双峰分离 + Z-crop紧凑度双重验证有效抑制假阳性",
    ]),
    ("问题", ORG, [
        "计量院背景过亮(lo_mean=21 vs 隧道5)，ratio被压低",
        "密集场景中3D聚类容差过大导致标靶并入大簇",
         "拓普康设备强度量纲不同，需单独适配",
    ]),
    ("下一步", GRN, [
        "自适应ratio阈值或局部对比度替代全局P80",
        "收窄聚类容差 / 密度聚类解决密集场景合并",
        "PCA圆形度(l1/l2)作为第三维过滤特征",
    ]),
]
y = 1.3
for title, color, items in sections:
    txt(s, title, 0.5, y, 2, 0.4, size=18, color=color, bold=True)
    y += 0.5
    for item in items:
        txt(s, "• " + item, 0.8, y, 12, 0.3, size=13, color=DK)
        y += 0.35
    y += 0.25

prs.save('output/report.pptx')
print('Saved output/report.pptx')
