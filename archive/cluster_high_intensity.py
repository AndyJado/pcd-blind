#!/usr/bin/env python3
"""高反射点聚类可视化 — 不做球拟合，纯看每个高反团块"""
import numpy as np
import os, sys
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib import font_manager
from sklearn.cluster import DBSCAN

# 中文字体
for f in font_manager.fontManager.ttflist:
    if 'Heiti' in f.name or 'PingFang' in f.name or 'Hei' in f.name:
        plt.rcParams['font.sans-serif'] = [f.name, 'DejaVu Sans']
        break
plt.rcParams['axes.unicode_minus'] = False

# ── PCD 解析 ────────────────────────────────────────────
def read_pcd(path):
    with open(path, 'rb') as f:
        header_lines = []
        for line in f:
            header_lines.append(line.decode('utf-8', errors='replace').rstrip('\n'))
            if line.startswith(b'DATA'):
                break
        offset = f.tell()
    header = {}
    for line in header_lines:
        if line.startswith('#') or not line.strip(): continue
        parts = line.split()
        if len(parts) >= 2: header[parts[0]] = ' '.join(parts[1:])
    fields = header['FIELDS'].split()
    sizes = [int(s) for s in header['SIZE'].split()]
    types = header['TYPE'].split()
    npoints = int(header['POINTS'])
    tm = {'F':'f','I':'i','U':'B'}
    dt = np.dtype([(f, tm[t]+str(s)) for f,s,t in zip(fields,sizes,types)])
    with open(path, 'rb') as f:
        f.seek(offset)
        data = np.fromfile(f, dtype=dt, count=npoints)
    return header, data

# ── 体素降采样 ──────────────────────────────────────────
def voxel_downsample(pts, voxel_size):
    mins = pts.min(axis=0)
    idx = np.floor((pts - mins) / voxel_size).astype(np.int64)
    _, uni = np.unique(idx, axis=0, return_index=True)
    return uni

# ── 主流程 ──────────────────────────────────────────────
if __name__ == '__main__':
    BASE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    path = sys.argv[1] if len(sys.argv) > 1 else os.path.join(BASE, "source", "绍兴白峰岭隧道.pcd")
    basename = os.path.splitext(os.path.basename(path))[0]
    outdir = os.path.join(BASE, "output", f"{basename}_clusters")
    os.makedirs(outdir, exist_ok=True)

    print(f"读取: {path}")
    header, data = read_pcd(path)
    n_total = len(data)
    print(f"  总点数: {n_total:,}")

    # ── 强度分析 ──
    intensity = data['intensity']
    i_threshold = np.percentile(intensity, 99.9)
    print(f"  Intensity 范围: [{intensity.min():.0f}, {intensity.max():.0f}], 阈值 P99.9={i_threshold:.0f}")

    high_mask = intensity >= i_threshold
    n_high = high_mask.sum()
    print(f"  高反射点: {n_high:,} ({100*n_high/n_total:.1f}%)")

    # ── 体素降采样 ──
    high_pts = np.column_stack([data['x'][high_mask], data['y'][high_mask], data['z'][high_mask]])
    high_intensity = intensity[high_mask]
    high_indices = np.where(high_mask)[0]

    voxel_size = 0.03  # 3cm 体素
    uni = voxel_downsample(high_pts, voxel_size)
    ds_pts = high_pts[uni]
    ds_intensity = high_intensity[uni]
    ds_indices = high_indices[uni]
    print(f"  体素降采样 ({voxel_size}m): {n_high:,} -> {len(ds_pts):,}")

    # ── DBSCAN 聚类 ──
    print("  DBSCAN 聚类 (eps=0.4m, min_samples=10)...")
    clustering = DBSCAN(eps=0.4, min_samples=10).fit(ds_pts)
    labels = clustering.labels_
    n_clusters = len(set(labels)) - (1 if -1 in labels else 0)
    n_noise = (labels == -1).sum()
    print(f"  聚类数: {n_clusters}, 噪声点: {n_noise}")

    # ── 收集每个 cluster 的统计 ──
    clusters = []
    for lid in set(labels):
        if lid == -1: continue
        mask = labels == lid
        c_pts = ds_pts[mask]
        c_int = ds_intensity[mask]
        c_idx = ds_indices[mask]
        n = len(c_pts)
        centroid = c_pts.mean(axis=0)
        clusters.append({
            'id': lid,
            'n': n,
            'centroid': centroid,
            'intensity_mean': c_int.mean(),
            'intensity_max': c_int.max(),
            'bbox': (c_pts.min(axis=0), c_pts.max(axis=0)),
            'pts': c_pts,
            'intensity': c_int,
            'orig_indices': c_idx,
        })

    clusters.sort(key=lambda c: c['n'], reverse=True)
    print(f"\n  共 {len(clusters)} 个有效聚类:")
    print(f"  {'ID':>4s} {'点数':>8s} {'重心X':>10s} {'重心Y':>10s} {'重心Z':>10s} {'强度均值':>10s} {'强度最大':>10s}")
    print("  " + "-"*70)
    for c in clusters:
        cx, cy, cz = c['centroid']
        print(f"  {c['id']:4d} {c['n']:8d} {cx:10.3f} {cy:10.3f} {cz:10.3f} "
              f"{c['intensity_mean']:10.1f} {c['intensity_max']:10.1f}")

    # ── 图1：全景点云 + 彩色聚类标注 ──
    print("\n  生成 overview 图...")
    step = max(1, n_total // 150000)
    idx = np.arange(0, n_total, step)
    xs = data['x'][idx]; ys = data['y'][idx]; zs = data['z'][idx]

    fig, axes = plt.subplots(1, 3, figsize=(30, 10))

    # 背景点云
    for ax_i, (ax, title, vx, vy, xl, yl) in enumerate([
        (axes[0], 'Top View (XY)', xs, ys, 'X (m)', 'Y (m)'),
        (axes[1], 'Side View (XZ)', xs, zs, 'X (m)', 'Z (m)'),
        (axes[2], 'Front View (YZ)', ys, zs, 'Y (m)', 'Z (m)'),
    ]):
        ax.scatter(vx, vy, c='#333333', s=0.3, alpha=0.4, rasterized=True)
        ax.set_xlabel(xl); ax.set_ylabel(yl); ax.set_title(title)
        ax.set_aspect('equal')

    # 每个 cluster 用不同颜色
    cmap = plt.cm.tab20
    colors = [cmap(i % 20) for i in range(len(clusters))]
    for i, c in enumerate(clusters):
        col = colors[i]
        pt = c['pts']
        for ax_i, (ax, vx, vy) in enumerate([
            (axes[0], pt[:,0], pt[:,1]),
            (axes[1], pt[:,0], pt[:,2]),
            (axes[2], pt[:,1], pt[:,2]),
        ]):
            ax.scatter(vx, vy, c=[col], s=8, alpha=0.9, edgecolors='none')
            ax.annotate(str(i), (vx.mean(), vy.mean()),
                       fontsize=7, fontweight='bold',
                       color='white', ha='center', va='center',
                       bbox=dict(boxstyle='round,pad=0.2', facecolor=col, alpha=0.8))

    fig.suptitle(f'{basename} — {len(clusters)} high-intensity clusters (P99.9>{i_threshold:.0f})',
                 fontsize=14, y=0.98)
    plt.tight_layout()
    overview_path = os.path.join(outdir, "00_overview.png")
    fig.savefig(overview_path, dpi=150, bbox_inches='tight')
    plt.close(fig)
    print(f"  -> {overview_path}")

    # ── 图2：每个 cluster 独立 plot ──
    print("\n  生成各 cluster 独立图...")
    n_cols = 4
    n_rows = (len(clusters) + n_cols - 1) // n_cols
    fig, axes = plt.subplots(n_rows, n_cols, figsize=(4*n_cols, 4*n_rows))
    if n_rows == 1:
        axes = axes.reshape(1, -1)

    for i, c in enumerate(clusters):
        row, col = i // n_cols, i % n_cols
        ax = axes[row][col]
        pt = c['pts']
        # 3D scatter projected to XY
        sc = ax.scatter(pt[:,0], pt[:,1], c=c['intensity'], s=10, cmap='hot',
                        edgecolors='none', alpha=0.8)
        cx, cy, cz = c['centroid']
        ax.scatter(cx, cy, c='cyan', s=60, marker='x', linewidths=2)
        ax.set_title(f"#{i} (n={c['n']})\n{cx:.2f}, {cy:.2f}, {cz:.2f}\n"
                     f"int: mean={c['intensity_mean']:.0f} max={c['intensity_max']:.0f}",
                     fontsize=8)
        ax.set_xlabel('X'); ax.set_ylabel('Y')
        ax.set_aspect('equal')
        # 固定比例尺
        span_x = pt[:,0].max() - pt[:,0].min()
        span_y = pt[:,1].max() - pt[:,1].min()
        margin = max(span_x, span_y) * 0.2 + 0.05
        ax.set_xlim(pt[:,0].min()-margin, pt[:,0].max()+margin)
        ax.set_ylim(pt[:,1].min()-margin, pt[:,1].max()+margin)
        plt.colorbar(sc, ax=ax, shrink=0.8)

    # 隐藏多余的子图
    for j in range(len(clusters), n_rows * n_cols):
        axes[j // n_cols][j % n_cols].set_visible(False)

    fig.suptitle(f'{basename} — Individual Clusters', fontsize=14, y=0.99)
    plt.tight_layout()
    clusters_path = os.path.join(outdir, "01_clusters_detail.png")
    fig.savefig(clusters_path, dpi=200, bbox_inches='tight')
    plt.close(fig)
    print(f"  -> {clusters_path}")

    # ── 保存 cluster 数据为 CSV ──
    csv_path = os.path.join(outdir, "clusters.csv")
    with open(csv_path, 'w') as f:
        f.write("id,n_points,cx,cy,cz,int_mean,int_max,"
                "x_min,x_max,y_min,y_max,z_min,z_max\n")
        for c in clusters:
            bb_min, bb_max = c['bbox']
            f.write(f"{c['id']},{c['n']},{c['centroid'][0]:.4f},{c['centroid'][1]:.4f},{c['centroid'][2]:.4f},"
                    f"{c['intensity_mean']:.2f},{c['intensity_max']:.2f},"
                    f"{bb_min[0]:.4f},{bb_max[0]:.4f},{bb_min[1]:.4f},{bb_max[1]:.4f},{bb_min[2]:.4f},{bb_max[2]:.4f}\n")
    print(f"  -> {csv_path}")

    print(f"\n完成! 输出: {outdir}/")
