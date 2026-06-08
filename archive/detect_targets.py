#!/usr/bin/env python3
"""隧道点云可视化 + 标靶球检测 (RANSAC sphere fitting)"""
import numpy as np
import struct, os, sys
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.patches import Circle
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

# ── RANSAC 球拟合 ──────────────────────────────────────
def fit_sphere_ransac(pts, thresh=0.02, max_iters=2000, min_inliers=30):
    """用 RANSAC 拟合球，返回 (cx,cy,cz,r) 或 None"""
    best = None
    best_n = 0
    n = len(pts)
    if n < 4: return None
    x2 = pts[:,0]**2 + pts[:,1]**2 + pts[:,2]**2
    ones = np.ones(n)
    for _ in range(max_iters):
        idx = np.random.choice(n, 4, replace=False)
        sample = pts[idx]
        A = np.c_[sample[:,:3], np.ones(4)]
        b = -(sample[:,0]**2 + sample[:,1]**2 + sample[:,2]**2)
        try:
            sol = np.linalg.solve(A, b)
        except np.linalg.LinAlgError:
            continue
        cx, cy, cz = -sol[0]/2, -sol[1]/2, -sol[2]/2
        r = np.sqrt(cx**2 + cy**2 + cz**2 - sol[3])
        if r <= 0 or r > 1.0:  # 标靶球半径通常在 0.05~0.15m
            continue
        # 统计内点（点到球面距离 < thresh）
        dists = np.abs(np.sqrt((pts[:,0]-cx)**2 + (pts[:,1]-cy)**2 + (pts[:,2]-cz)**2) - r)
        inliers = dists < thresh
        n_in = inliers.sum()
        # 要求至少 20% 的内点率（球面覆盖）
        coverage = n_in / (4*np.pi*r*r) if r > 0 else 0
        if n_in > best_n and coverage > 500:  # 大约每平方米 500 点
            best_n = n_in
            best = (cx, cy, cz, r, inliers)
            if n_in > n * 0.6:
                break
    if best is None: return None
    # 精化：用内点最小二乘重算
    mask = best[4]
    in_pts = pts[mask]
    if len(in_pts) < 4: return None
    A = np.c_[in_pts[:,:3], np.ones(len(in_pts))]
    b = -(in_pts[:,0]**2 + in_pts[:,1]**2 + in_pts[:,2]**2)
    sol, _, _, _ = np.linalg.lstsq(A, b, rcond=None)
    cx, cy, cz = -sol[0]/2, -sol[1]/2, -sol[2]/2
    r = np.sqrt(max(0, cx**2 + cy**2 + cz**2 - sol[3]))
    return (cx, cy, cz, r, mask)

# ── 绘图 ────────────────────────────────────────────────
def plot_pcd(data, targets, out_path, title):
    x, y, z, intensity = data['x'], data['y'], data['z'], data['intensity']

    # 降采样
    step = max(1, len(data) // 200000)
    idx = np.arange(0, len(data), step)
    xs, ys, zs = x[idx].astype(np.float64), y[idx].astype(np.float64), z[idx].astype(np.float64)
    is_ = intensity[idx].astype(np.float64)

    fig = plt.figure(figsize=(28, 10))

    # ── 顶视图 XY ──
    ax1 = fig.add_subplot(1, 3, 1)
    sc1 = ax1.scatter(xs, ys, c=is_, s=0.5, cmap='hot', alpha=0.6, rasterized=True)
    ax1.set_xlabel('X (m)'); ax1.set_ylabel('Y (m)'); ax1.set_title('Top View (XY)')
    ax1.set_aspect('equal')
    plt.colorbar(sc1, ax=ax1, label='Intensity')

    # ── 侧视图 XZ ──
    ax2 = fig.add_subplot(1, 3, 2)
    sc2 = ax2.scatter(xs, zs, c=is_, s=0.5, cmap='hot', alpha=0.6, rasterized=True)
    ax2.set_xlabel('X (m)'); ax2.set_ylabel('Z (m)'); ax2.set_title('Side View (XZ)')
    ax2.set_aspect('equal')
    plt.colorbar(sc2, ax=ax2, label='Intensity')

    # ── 3D 视图 ──
    ax3 = fig.add_subplot(1, 3, 3, projection='3d')
    step3d = max(1, len(data) // 80000)
    idx3 = np.arange(0, len(data), step3d)
    xs3, ys3, zs3 = x[idx3].astype(np.float64), y[idx3].astype(np.float64), z[idx3].astype(np.float64)
    is3 = intensity[idx3].astype(np.float64)
    sc3 = ax3.scatter(xs3, ys3, zs3, c=is3, s=0.3, cmap='hot', alpha=0.4, rasterized=True)
    ax3.set_xlabel('X (m)'); ax3.set_ylabel('Y (m)'); ax3.set_zlabel('Z (m)')
    ax3.set_title('3D View')

    # ── 标出检测到的标靶球 ──
    colors = ['cyan', 'lime', 'yellow', 'magenta', 'orange', 'white']
    for i, t in enumerate(targets):
        cx, cy, cz, r = t['cx'], t['cy'], t['cz'], t['r']
        col = colors[i % len(colors)]
        label = f"#{i}: r={r*100:.1f}cm"
        for ax in [ax1, ax2, ax3]:
            if ax == ax1:
                ax.scatter(cx, cy, c=col, s=80, marker='x', linewidths=2, label=label)
                c = Circle((cx, cy), r, fill=False, color=col, linewidth=1.5)
                ax.add_patch(c)
                ax.legend(loc='upper right', fontsize=7)
            elif ax == ax2:
                ax.scatter(cx, cz, c=col, s=80, marker='x', linewidths=2)
                c = Circle((cx, cz), r, fill=False, color=col, linewidth=1.5)
                ax.add_patch(c)
            elif ax == ax3:
                ax.scatter(cx, cy, cz, c=col, s=80, marker='x', linewidths=3)

    fig.suptitle(title, fontsize=13, y=0.98)
    plt.tight_layout()
    print(f"  -> 保存: {out_path}")
    fig.savefig(out_path, dpi=150, bbox_inches='tight')
    plt.close(fig)

def plot_intensity_hist(data, out_path):
    """intensity 直方图"""
    fig, ax = plt.subplots(figsize=(10, 4))
    ax.hist(data['intensity'], bins=256, range=(0,255), color='orange', alpha=0.7)
    ax.set_xlabel('Intensity'); ax.set_ylabel('Count')
    ax.set_title('Intensity Distribution')
    ax.set_yscale('log')
    # 标注分位数
    for p, color in [(90,'red'),(95,'red'),(97,'blue'),(99,'blue')]:
        v = np.percentile(data['intensity'], p)
        ax.axvline(v, color=color, linestyle='--', alpha=0.6)
        ax.text(v, ax.get_ylim()[1]*0.5, f'P{p}={v:.0f}', rotation=90, fontsize=8, color=color)
    fig.tight_layout()
    fig.savefig(out_path, dpi=120)
    plt.close(fig)
    print(f"  -> 保存: {out_path}")

# ── 主流程 ──────────────────────────────────────────────
if __name__ == '__main__':
    BASE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    path = sys.argv[1] if len(sys.argv) > 1 else os.path.join(BASE, "source", "绍兴白峰岭隧道.pcd")
    basename = os.path.splitext(os.path.basename(path))[0]
    outdir = os.path.join(BASE, "output", f"{basename}_analysis")
    os.makedirs(outdir, exist_ok=True)

    print(f"读取: {path}")
    header, data = read_pcd(path)
    print(f"  点数: {len(data):,}")

    # ── 1. Intensity 直方图 ──
    print("\n[1/4] Intensity 分布图...")
    plot_intensity_hist(data, os.path.join(outdir, "intensity_hist.png"))

    # ── 2. 检测标靶球 ──
    print("\n[2/4] 检测标靶球...")
    # 标靶球反射率通常很高，先取高 intensity 点
    i_threshold = np.percentile(data['intensity'], 95)
    print(f"  Intensity 阈值 (P95): {i_threshold:.0f}")
    high_mask = data['intensity'] >= i_threshold
    high_pts = np.column_stack([data['x'][high_mask], data['y'][high_mask], data['z'][high_mask]])
    print(f"  高反射点: {len(high_pts):,} / {len(data):,} ({100*len(high_pts)/len(data):.1f}%)")

    # 体素降采样（点数太多时 DBSCAN 会 OOM）
    MAX_DBSCAN_PTS = 300000
    if len(high_pts) > MAX_DBSCAN_PTS:
        voxel_size = 0.05  # 5cm 体素
        print(f"  体素降采样 (voxel={voxel_size}m, {len(high_pts):,} -> ...")
        mins = high_pts.min(axis=0)
        voxel_idx = np.floor((high_pts - mins) / voxel_size).astype(np.int64)
        _, unique_idx = np.unique(voxel_idx, axis=0, return_index=True)
        dbscan_pts = high_pts[unique_idx]
        # 保留 intensity 映射回原始索引
        dbscan_intensity = data['intensity'][high_mask][unique_idx]
        print(f"  -> {len(dbscan_pts):,} 点")
    else:
        dbscan_pts = high_pts
        dbscan_intensity = data['intensity'][high_mask]

    # DBSCAN 聚类
    print("  DBSCAN 聚类...")
    # 标靶球通常彼此距离较远，eps 设大一点
    clustering = DBSCAN(eps=0.5, min_samples=20).fit(dbscan_pts)
    labels = clustering.labels_
    n_clusters = len(set(labels)) - (1 if -1 in labels else 0)
    n_noise = (labels == -1).sum()
    print(f"  聚类数: {n_clusters}, 噪声点: {n_noise}")

    # 对每个聚类拟合球
    targets = []
    for label in set(labels):
        if label == -1: continue
        cluster_pts = dbscan_pts[labels == label]
        if len(cluster_pts) < 30: continue
        # 尝试多个 intensity 阈值来找球
        best_result = None
        for frac in [1.0, 0.5, 0.3]:
            subset = cluster_pts
            if frac < 1.0:
                ci = np.percentile(dbscan_intensity[labels==label], 100*(1-frac))
                sub = dbscan_pts[labels==label]
                subset = sub[dbscan_intensity[labels==label] >= ci]
            if len(subset) < 10: continue
            result = fit_sphere_ransac(subset, thresh=0.015, max_iters=3000, min_inliers=20)
            if result:
                cx, cy, cz, r, in_mask = result
                # 只接受合理半径的球（标靶球通常 7-25cm）
                if 0.07 <= r <= 0.25:
                    best_result = (cx, cy, cz, r, len(subset[in_mask]))
                    break
        if best_result:
            cx, cy, cz, r, nin = best_result
            # 计算 cluster 的 intensity 均值
            cint = dbscan_intensity[labels==label].mean()
            targets.append({'cx':cx,'cy':cy,'cz':cz,'r':r,'nin':nin,'intensity':cint,
                           'cluster_id':label,'cluster_size':len(cluster_pts)})
            # 额外过滤：标靶球通常在隧道两侧/地面，不是在天花板上
            if cz > 5.0 or nin < 30:
                continue
            print(f"  Cluster {label}: center=({cx:.3f},{cy:.3f},{cz:.3f}), r={r*100:.1f}cm, "
                  f"inliers={nin}, cluster_size={len(cluster_pts)}, "
                  f"intensity_avg={cint:.1f}")

    # 去重：合并中心距离很近的球
    if len(targets) > 1:
        merged = []
        used = set()
        for i, t1 in enumerate(targets):
            if i in used: continue
            group = [t1]
            for j, t2 in enumerate(targets):
                if j <= i or j in used: continue
                dist = np.sqrt((t1['cx']-t2['cx'])**2+(t1['cy']-t2['cy'])**2+(t1['cz']-t2['cz'])**2)
                if dist < 0.3:
                    group.append(t2)
                    used.add(j)
            used.add(i)
            if len(group) == 1:
                merged.append(group[0])
            else:
                avg = {k: np.mean([t[k] for t in group]) if k not in ('cluster_id',) else group[0][k]
                       for k in ['cx','cy','cz','r','nin','intensity']}
                avg['cluster_size'] = sum(t['cluster_size'] for t in group)
                merged.append(avg)
        targets = merged

    targets.sort(key=lambda t: t['cx'])
    print(f"\n  最终检测到 {len(targets)} 个标靶球:")

    if targets:
        print(f"  {'#':>3s} {'X(m)':>10s} {'Y(m)':>10s} {'Z(m)':>10s} {'半径cm':>8s} {'强度均值':>10s}")
        print("  " + "-"*55)
        for i, t in enumerate(targets):
            print(f"  {i:3d} {t['cx']:10.3f} {t['cy']:10.3f} {t['cz']:10.3f} "
                  f"{t['r']*100:8.1f} {t['intensity']:10.1f}")
    else:
        print("  未检测到标靶球")

    # ── 3. 整体点云图 ──
    print("\n[3/4] 点云可视化...")
    plot_pcd(data, targets, os.path.join(outdir, "overview.png"),
             f"{basename} ({len(data):,} pts)")

    # ── 4. 高反射点分布图 ──
    print("\n[4/4] 高反射点分布...")
    high_data = np.zeros(len(data), dtype=[('x','f4'),('y','f4'),('z','f4'),('intensity','f4')])
    for f in ['x','y','z']:
        high_data[f] = data[f]
    high_data['intensity'] = data['intensity']
    # 只看高反射区域的一个窗口
    hi = high_mask
    window_data = high_data[hi & (data['x'] > data['x'].min()+5) & (data['x'] < data['x'].max()-5)]
    if len(window_data) < 5000:
        window_data = high_data[hi]
    plot_pcd(window_data, targets, os.path.join(outdir, "high_intensity.png"),
             f"{basename} - High Intensity Points (>{i_threshold:.0f}, {len(window_data):,} pts)")

    print(f"\n完成! 输出目录: {outdir}")
    print(f"  ls {outdir}")
