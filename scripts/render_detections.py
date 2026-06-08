#!/usr/bin/env python3
"""render_detections.py — 读取 PCD + CSV，生成标注 3D 截图"""
import struct, os, sys, csv, numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D

def read_pcd(path):
    """简易 PCD reader for PointXYZI (binary)"""
    with open(path, 'rb') as f:
        header = []
        while True:
            line = f.readline().decode('utf-8', errors='replace').strip()
            header.append(line)
            if line.startswith('DATA'):
                break
        fmt = [l for l in header if l.startswith('FIELDS')][0]
        has_i = 'intensity' in fmt.lower() or 'I' in fmt.split()[1:]
        data_type = [l for l in header if l.startswith('DATA')][0]
        binary = 'binary' in data_type
        if not binary:
            return None
        # Read binary points
        buf = f.read()
        # PointXYZI = 4 floats (x,y,z,intensity) = 16 bytes
        n = len(buf) // 16
        pts = np.frombuffer(buf, dtype=np.float32).reshape(-1, 4)
    return pts  # [x, y, z, intensity]

def main():
    dirs = [
        ('output/白峰岭_detect_v13', 'Baifengling'),
        ('output/庆元1_detect_v13', 'Qingyuan-1'),
        ('output/庆元2_detect_v13', 'Qingyuan-2'),
        ('output/庆元3_detect_v13', 'Qingyuan-3'),
        ('output/计量院_detect_v13', 'Jiliangyuan'),
    ]

    for d, name in dirs:
        csv_path = os.path.join(d, 'results.csv')
        if not os.path.exists(csv_path):
            continue
        ss_dir = os.path.join(d, 'screenshots')
        os.makedirs(ss_dir, exist_ok=True)

        with open(csv_path) as f:
            reader = csv.DictReader(f)
            for row in reader:
                rank = int(row['rank'])
                ratio = float(row['ratio'])
                compact = float(row['compact'])
                cx, cy, cz = float(row['cx']), float(row['cy']), float(row['cz'])

                # Find matching PCD
                match_file = None
                for fn in os.listdir(d):
                    if fn.startswith('match_') and fn.endswith('.pcd'):
                        pts = read_pcd(os.path.join(d, fn))
                        if pts is None:
                            continue
                        hi_pts = pts[pts[:,3] == 255]
                        if len(hi_pts) == 0:
                            continue
                        mx, my, mz = hi_pts[:,:3].mean(axis=0)
                        if np.sqrt((mx-cx)**2 + (my-cy)**2 + (mz-cz)**2) < 0.5:
                            match_file = os.path.join(d, fn)
                            break
                if match_file is None:
                    continue

                pts = read_pcd(match_file)
                if pts is None:
                    continue

                # Split by intensity
                lo = pts[pts[:,3] <= 100]
                hi_crop = pts[pts[:,3] == 255]   # red
                hi_below = pts[(pts[:,3] > 100) & (pts[:,3] < 255)]  # orange

                # 3D plot
                fig = plt.figure(figsize=(10, 8))
                ax = fig.add_subplot(111, projection='3d')
                ax.set_facecolor('#1a1a25')
                fig.patch.set_facecolor('#1a1a25')

                if len(lo) > 0:
                    ax.scatter(lo[:,0], lo[:,1], lo[:,2], c='#3c50c8', s=1, alpha=0.4, label='lo')
                if len(hi_below) > 0:
                    ax.scatter(hi_below[:,0], hi_below[:,1], hi_below[:,2], c='#e8a030', s=3, alpha=0.5, label='hi(below)')
                if len(hi_crop) > 0:
                    ax.scatter(hi_crop[:,0], hi_crop[:,1], hi_crop[:,2], c='#ff3c28', s=6, label='hi(crop)')

                # Sphere marker
                u, v = np.mgrid[0:2*np.pi:20j, 0:np.pi:10j]
                r = 0.1
                sx = cx + r * np.cos(u) * np.sin(v)
                sy = cy + r * np.sin(u) * np.sin(v)
                sz = cz + r * np.cos(v)
                ax.plot_wireframe(sx, sy, sz, color='#00ff88', linewidth=0.5, alpha=0.6)

                ax.scatter([cx], [cy], [cz], c='#00ff88', s=50, marker='o')

                # Labels
                ax.set_xlabel('X')
                ax.set_ylabel('Y')
                ax.set_zlabel('Z')
                ax.set_title(f'{name} #{rank}  r={ratio:.1f}  c={compact:.2f}\n({cx:.2f}, {cy:.2f}, {cz:.2f})',
                             color='white', fontsize=11)
                ax.tick_params(colors='white')
                ax.xaxis.label.set_color('white')
                ax.yaxis.label.set_color('white')
                ax.zaxis.label.set_color('white')

                # View
                ax.view_init(elev=25, azim=-45)

                # Equal aspect
                all_pts = pts[:,:3]
                ranges = all_pts.max(axis=0) - all_pts.min(axis=0)
                mid = all_pts.mean(axis=0)
                max_range = ranges.max() * 0.6
                ax.set_xlim(mid[0]-max_range, mid[0]+max_range)
                ax.set_ylim(mid[1]-max_range, mid[1]+max_range)
                ax.set_zlim(mid[2]-max_range, mid[2]+max_range)

                fn = os.path.join(ss_dir, f'rank{rank:02d}_r{ratio:.0f}.png')
                plt.savefig(fn, dpi=150, bbox_inches='tight', facecolor='#1a1a25')
                plt.close()
                print(f'{name} rank#{rank} → {fn}')

if __name__ == '__main__':
    main()
