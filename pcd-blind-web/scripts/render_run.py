#!/usr/bin/env python3
"""render_run.py — 为 Web 前端生成检测结果截图 (PNG)"""
import csv, os, sys, struct
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

def read_pcd(path):
    with open(path, 'rb') as f:
        header = []
        while True:
            line = f.readline().decode('utf-8', errors='replace').strip()
            header.append(line)
            if line.startswith('DATA'):
                break
        buf = f.read()
        n = len(buf) // 16
        pts = np.frombuffer(buf, dtype=np.float32).reshape(-1, 4)
    return pts

def render_run(run_dir):
    results_csv = os.path.join(run_dir, 'results.csv')
    ss_dir = os.path.join(run_dir, 'screenshots')
    os.makedirs(ss_dir, exist_ok=True)

    if not os.path.exists(results_csv):
        return []

    generated = []
    with open(results_csv) as f:
        reader = csv.DictReader(f)
        for row in reader:
            rank = int(row['rank'])
            cx = float(row['cx'])
            cy = float(row['cy'])
            cz = float(row['cz'])
            ratio = float(row.get('ratio', 0))
            tripod = float(row.get('tripod', 0))
            compact = float(row.get('compact', 0))

            # 找对应的 match PCD
            match_file = None
            for fn in sorted(os.listdir(run_dir)):
                if fn.startswith('match_') and fn.endswith('.pcd'):
                    try:
                        pts = read_pcd(os.path.join(run_dir, fn))
                    except Exception:
                        continue
                    hi = pts[pts[:, 3] == 255]
                    if len(hi) < 10:
                        continue
                    mx, my, mz = hi[:, :3].mean(axis=0)
                    if np.sqrt((mx - cx)**2 + (my - cy)**2 + (mz - cz)**2) < 0.8:
                        match_file = fn
                        break

            if match_file is None:
                continue

            try:
                pts = read_pcd(os.path.join(run_dir, match_file))
            except Exception:
                continue

            lo = pts[pts[:, 3] <= 100]
            hi = pts[pts[:, 3] == 255]
            mid_pts = pts[(pts[:, 3] > 100) & (pts[:, 3] < 255)]

            fig = plt.figure(figsize=(7, 5))
            ax = fig.add_subplot(111, projection='3d')
            fig.patch.set_facecolor('#0d1117')
            ax.set_facecolor('#0d1117')

            if len(lo) > 0:
                ax.scatter(lo[:, 0], lo[:, 1], lo[:, 2],
                           c='#3c50c8', s=0.5, alpha=0.35)
            if len(mid_pts) > 0:
                ax.scatter(mid_pts[:, 0], mid_pts[:, 1], mid_pts[:, 2],
                           c='#e8a030', s=1.5, alpha=0.4)
            if len(hi) > 0:
                ax.scatter(hi[:, 0], hi[:, 1], hi[:, 2],
                           c='#ff3c28', s=4, alpha=0.9)

            # 球体线框
            u, v = np.mgrid[0:2*np.pi:16j, 0:np.pi:8j]
            r = 0.1
            sx = cx + r * np.cos(u) * np.sin(v)
            sy = cy + r * np.sin(u) * np.sin(v)
            sz = cz + r * np.cos(v)
            ax.plot_wireframe(sx, sy, sz, color='#00ff88', linewidth=0.4, alpha=0.5)
            ax.scatter([cx], [cy], [cz], c='#00ff88', s=25)

            all_xyz = pts[:, :3]
            mid = all_xyz.mean(axis=0)
            rng = (all_xyz.max(axis=0) - all_xyz.min(axis=0)).max() * 0.55
            if rng < 0.3:
                rng = 0.3
            ax.set_xlim(mid[0] - rng, mid[0] + rng)
            ax.set_ylim(mid[1] - rng, mid[1] + rng)
            ax.set_zlim(mid[2] - rng, mid[2] + rng)
            ax.axis('off')
            ax.view_init(elev=20, azim=-50)

            out = os.path.join(ss_dir, f'{rank:02d}.png')
            plt.savefig(out, dpi=100, bbox_inches='tight', facecolor='#0d1117', pad_inches=0.1)
            plt.close()

            generated.append({
                'rank': rank, 'file': f'{rank:02d}.png',
                'cx': cx, 'cy': cy, 'cz': cz,
                'ratio': ratio, 'tripod': tripod, 'compact': compact
            })

    return generated

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: render_run.py <run_dir>")
        sys.exit(1)
    result = render_run(sys.argv[1])
    for r in result:
        print(f"  #{r['rank']:02d} → {r['file']} ({r['cx']:.2f},{r['cy']:.2f},{r['cz']:.2f})")
    print(f"Generated {len(result)} screenshots")
