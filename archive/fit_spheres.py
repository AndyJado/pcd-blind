#!/usr/bin/env python3
"""固定半径 RANSAC 球心搜索（r=10cm 已知）"""
import numpy as np, os, sys
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib import font_manager
from sklearn.cluster import DBSCAN

for f in font_manager.fontManager.ttflist:
    if 'Heiti' in f.name or 'PingFang' in f.name or 'Hei' in f.name:
        plt.rcParams['font.sans-serif'] = [f.name, 'DejaVu Sans']
        break
plt.rcParams['axes.unicode_minus'] = False

def read_pcd(path):
    with open(path, 'rb') as f:
        hdr = []
        for line in f:
            hdr.append(line.decode(errors='replace').rstrip('\n'))
            if line.startswith(b'DATA'): break
        off = f.tell()
    d = {}
    for l in hdr:
        if l.startswith('#') or not l.strip(): continue
        p = l.split(); d[p[0]] = ' '.join(p[1:]) if len(p)>=2 else ''
    fields = d['FIELDS'].split()
    sizes = [int(s) for s in d['SIZE'].split()]
    types = d['TYPE'].split()
    npts = int(d['POINTS'])
    tm = {'F':'f','I':'i','U':'B'}
    dt = np.dtype([(f, tm[t]+str(s)) for f,s,t in zip(fields,sizes,types)])
    with open(path, 'rb') as f:
        f.seek(off)
        return d, np.fromfile(f, dtype=dt, count=npts)

def fit_sphere_fixed_r(pts, R, thresh=0.015, max_iter=5000, min_inliers=20):
    """
    固定半径 R 的 RANSAC 球心搜索。
    每次随机取 4 点解球心（两解取离 pts 更近的），统计距离 R 容差内的点。
    """
    n = len(pts)
    if n < 4: return None
    best_center, best_inliers, best_n = None, None, 0
    pts_f64 = pts.astype(np.float64)
    for _ in range(min(max_iter, max(100, n*n//10))):
        idx = np.random.choice(n, 4, replace=False)
        sample = pts_f64[idx]
        # 解线性方程组求球心: 2(p_j-p_0)·c = ||p_j||² - ||p_0||²
        p0 = sample[0]
        A = 2 * (sample[1:] - p0)  # 3x3
        b = np.sum(sample[1:]**2, axis=1) - np.sum(p0**2)
        try:
            c = np.linalg.solve(A, b)
        except np.linalg.LinAlgError:
            continue
        # 验证: ||p0-c|| 应该 ≈ R
        if abs(np.linalg.norm(p0 - c) - R) > R * 0.5:
            continue
        # 统计 inliers
        dists = np.abs(np.linalg.norm(pts_f64 - c, axis=1) - R)
        inliers = dists < thresh
        n_in = inliers.sum()
        if n_in > best_n:
            best_n = n_in
            best_center = c
            best_inliers = inliers
    if best_n < min_inliers:
        return None
    # 精化：用 inlier 最小二乘优化球心
    in_pts = pts_f64[best_inliers]
    # 最小化 Σ(||p_i - c||² - R²)² → 对 c 求导得线性方程
    # ∇ Σ(||p_i||² - 2p_i·c + ||c||² - R²)² = 0
    # 近似：固定 ||c||² 用迭代，或直接解线性: 2p_i·(c_new - c_old) = ||p_i||² + ||c_old||² - 2p_i·c_old - R²...
    # 简单做法：用 scipy optimize 或直接取 inlier 的均值附近的精细网格搜索
    # 这里用简单迭代 LS：
    c_refined = best_center.copy()
    for _ in range(10):
        dists_r = np.linalg.norm(in_pts - c_refined, axis=1)
        directions = (in_pts - c_refined) / dists_r[:, None]
        targets = c_refined + directions * R
        c_new = targets.mean(axis=0)
        if np.linalg.norm(c_new - c_refined) < 1e-5:
            break
        c_refined = c_new
    # 重新统计
    dists = np.abs(np.linalg.norm(pts_f64 - c_refined, axis=1) - R)
    final_in = dists < thresh
    return c_refined, final_in.sum(), final_in

if __name__ == '__main__':
    BASE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    path = sys.argv[1] if len(sys.argv) > 1 else os.path.join(BASE, "source", "绍兴白峰岭隧道.pcd")
    basename = os.path.splitext(os.path.basename(path))[0]
    outdir = os.path.join(BASE, "output", f"{basename}_spheres_p99")
    os.makedirs(outdir, exist_ok=True)

    print(f"读取: {path}")
    _, data = read_pcd(path)
    n_total = len(data)
    print(f"  总点数: {n_total:,}")

    intensity = data['intensity']
    pct = 99
    i_threshold = np.percentile(intensity, pct)
    print(f"  P{pct} 阈值: {i_threshold:.0f}")

    high_mask = intensity >= i_threshold
    high_pts = np.column_stack([data['x'][high_mask], data['y'][high_mask], data['z'][high_mask]])
    high_intensity = intensity[high_mask]
    print(f"  高反射点: {len(high_pts):,} ({100*len(high_pts)/n_total:.2f}%)")

    # DBSCAN
    print("  DBSCAN (eps=0.5, min_samples=20)...")
    clustering = DBSCAN(eps=0.5, min_samples=20).fit(high_pts)
    labels = clustering.labels_
    n_clusters = len(set(labels)) - (1 if -1 in labels else 0)
    print(f"  聚类数: {n_clusters}, 噪声: {(labels==-1).sum()}")

    # 固定半径 RANSAC
    R = 0.10  # 200mm 直径
    print(f"\n  固定半径 RANSAC (r={R*100:.0f}cm, thresh=1.5cm)...")

    targets = []
    for lid in set(labels):
        if lid == -1: continue
        mask = labels == lid
        c_pts = high_pts[mask]
        c_int = high_intensity[mask]
        if len(c_pts) < 30: continue

        result = fit_sphere_fixed_r(c_pts, R, thresh=0.015, min_inliers=15)
        if result is None: continue
        center, n_in, in_mask = result
        if n_in < 15: continue

        targets.append({
            'id': lid,
            'cx': center[0], 'cy': center[1], 'cz': center[2],
            'r': R,
            'n_inliers': n_in,
            'n_total': len(c_pts),
            'intensity_mean': c_int.mean(),
            'intensity_max': c_int.max(),
            'pts': c_pts[in_mask],
        })

    targets.sort(key=lambda t: t['n_inliers'], reverse=True)
    print(f"\n  检测到 {len(targets)} 个标靶球:")
    print(f"  {'#':>3s} {'X(m)':>10s} {'Y(m)':>10s} {'Z(m)':>10s} {'内点数':>8s} {'总点数':>8s} {'内点率':>8s} {'强度均值':>10s}")
    print("  " + "-"*75)
    for i, t in enumerate(targets):
        print(f"  {i:3d} {t['cx']:10.3f} {t['cy']:10.3f} {t['cz']:10.3f} "
              f"{t['n_inliers']:8d} {t['n_total']:8d} {t['n_inliers']/t['n_total']*100:7.1f}% "
              f"{t['intensity_mean']:10.1f}")

    # ── 出图 ──
    print("\n  生成 overview...")
    step = max(1, n_total // 150000)
    idx = np.arange(0, n_total, step)
    xs, ys, zs = data['x'][idx], data['y'][idx], data['z'][idx]

    fig, axes = plt.subplots(1, 3, figsize=(30, 10))
    for ax_i, (ax, title, vx, vy, xl, yl) in enumerate([
        (axes[0], 'Top View (XY)', xs, ys, 'X (m)', 'Y (m)'),
        (axes[1], 'Side View (XZ)', xs, zs, 'X (m)', 'Z (m)'),
        (axes[2], 'Front View (YZ)', ys, zs, 'Y (m)', 'Z (m)'),
    ]):
        ax.scatter(vx, vy, c='#222', s=0.3, alpha=0.35, rasterized=True)
        ax.set_xlabel(xl); ax.set_ylabel(yl); ax.set_title(title); ax.set_aspect('equal')

    colors = ['cyan', 'lime', 'yellow', 'magenta', 'orange', 'red', 'white', 'dodgerblue']
    for i, t in enumerate(targets):
        col = colors[i % len(colors)]
        pt = t['pts']
        cx, cy, cz = t['cx'], t['cy'], t['cz']
        for ax_i, (ax, vx, vy, cvx, cvy) in enumerate([
            (axes[0], pt[:,0], pt[:,1], cx, cy),
            (axes[1], pt[:,0], pt[:,2], cx, cz),
            (axes[2], pt[:,1], pt[:,2], cy, cz),
        ]):
            ax.scatter(vx, vy, c=col, s=10, alpha=0.9, edgecolors='none')
            ax.scatter(cvx, cvy, c=col, s=150, marker='x', linewidths=3)
            from matplotlib.patches import Circle
            ax.add_patch(Circle((cvx, cvy), R, fill=False, color=col, linewidth=2, ls='--'))
            ax.annotate(f"#{i}", (cvx, cvy+R+0.3), fontsize=9, fontweight='bold',
                       color=col, ha='center', va='bottom')

    fig.suptitle(f'{basename} — {len(targets)} targets (P{pct}, r={R*100:.0f}cm fixed)',
                 fontsize=14, y=0.98)
    plt.tight_layout()
    overview_path = os.path.join(outdir, "overview.png")
    fig.savefig(overview_path, dpi=150, bbox_inches='tight')
    plt.close(fig)

    # CSV
    csv_path = os.path.join(outdir, "spheres.csv")
    with open(csv_path, 'w') as f:
        f.write("id,cx,cy,cz,radius_m,n_inliers,n_total,inlier_pct,int_mean,int_max\n")
        for i, t in enumerate(targets):
            f.write(f"{i},{t['cx']:.4f},{t['cy']:.4f},{t['cz']:.4f},"
                    f"{t['r']:.3f},{t['n_inliers']},{t['n_total']},"
                    f"{t['n_inliers']/t['n_total']:.3f},"
                    f"{t['intensity_mean']:.1f},{t['intensity_max']:.0f}\n")
    print(f"  -> {csv_path}")
    print(f"\n完成! {outdir}/")
