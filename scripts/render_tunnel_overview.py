#!/usr/bin/env python3
"""render_tunnel_overview.py — 隧道全景点云XY投影 + 强度着色 + 检出标记"""
import struct, os, csv, glob, numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

datasets = [
    ('Baifengling Tunnel', 'source/绍兴白峰岭隧道.pcd', 'output/白峰岭_detect_v13'),
    ('Qingyuan Tunnel 1', 'source/庆元陈家岭隧道1.pcd', 'output/庆元1_detect_v13'),
    ('Qingyuan Tunnel 2', 'source/庆元陈家岭隧道2.pcd', 'output/庆元2_detect_v13'),
    ('Qingyuan Tunnel 3', 'source/庆元陈家岭隧道3.pcd', 'output/庆元3_detect_v13'),
    ('Jiliangyuan Lab', 'source/浙江省计量院一楼实验室.pcd', 'output/计量院_detect_v13'),
]

def read_pcd_xyi(path, max_pts=500000):
    """Read PCD, return (x, y, intensity) arrays, randomly subsampled"""
    with open(path, 'rb') as f:
        while True:
            line = f.readline().decode('utf-8', errors='replace').strip()
            if line.startswith('DATA'): break
        buf = f.read()
    n = len(buf) // 16
    pts = np.frombuffer(buf, dtype=np.float32).reshape(-1, 4)  # x,y,z,intensity
    if n > max_pts:
        idx = np.random.choice(n, max_pts, replace=False)
        pts = pts[idx]
    return pts[:, 0], pts[:, 1], pts[:, 3]

for name, src, out_d in datasets:
    if not os.path.exists(src): continue

    print(f"Rendering {name}...")
    x, y, intensity = read_pcd_xyi(src, max_pts=300000)

    fig, ax = plt.subplots(figsize=(14, 10), facecolor='white')
    ax.set_facecolor('white')

    # Scatter: color by intensity
    sc = ax.scatter(x, y, c=intensity, s=0.3, cmap='hot', alpha=0.7, rasterized=True)
    cbar = plt.colorbar(sc, ax=ax, shrink=0.8)
    cbar.set_label('Intensity', fontsize=10)

    # Plot detections
    csv_path = os.path.join(out_d, 'results.csv')
    if os.path.exists(csv_path):
        with open(csv_path) as f:
            reader = list(csv.DictReader(f))
        for row in reader[:10]:
            cx, cy = float(row['cx']), float(row['cy'])
            ratio = float(row['ratio'])
            rank = int(row['rank'])
            color = 'lime' if rank < 4 else 'cyan'
            ax.plot(cx, cy, 'o', markersize=8 if rank < 4 else 5,
                    markerfacecolor=color, markeredgecolor='black', markeredgewidth=0.5)
            if rank < 5:
                ax.annotate(f'#{rank}\nr={ratio:.1f}', (cx, cy),
                           textcoords="offset points", xytext=(5, 5),
                           fontsize=7, color='black',
                           bbox=dict(boxstyle='round,pad=0.2', facecolor='white', alpha=0.8))

    ax.set_xlabel('X (m)', fontsize=11)
    ax.set_ylabel('Y (m)', fontsize=11)
    ax.set_title(f'{name} ({len(x):,} pts)', fontsize=14, fontweight='bold')
    ax.set_aspect('equal')

    # Grid
    ax.grid(True, alpha=0.2, color='gray')
    ax.tick_params(labelsize=9)

    # Save
    out_path = os.path.join(out_d, 'overview.png')
    plt.savefig(out_path, dpi=200, bbox_inches='tight', facecolor='white')
    plt.close()
    print(f"  → {out_path}")

print("Done.")
