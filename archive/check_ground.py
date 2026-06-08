import struct, numpy as np
path = "/home/mz/mainframer/pcd-blind/source/绍兴白峰岭隧道.pcd"
with open(path,'rb') as f:
    for l in f:
        if l.startswith(b'DATA'): break
    off=f.tell()
dt=np.dtype([('x','f4'),('y','f4'),('z','f4'),('i','f4'),('nx','f4'),('ny','f4'),('nz','f4'),('c','f4')])
with open(path,'rb') as f:
    f.seek(off); d=np.fromfile(f,dtype=dt)

targets = [("T1",32.85,8.12),("T2",19.29,-4.04),("T3",33.08,-4.59),("T4",21.67,8.11)]
for label,cx,cy in targets:
    near=d[(abs(d['x']-cx)<1.5)&(abs(d['y']-cy)<1.5)]
    if len(near)>0:
        zs = near['z']
        print(f"{label}: Z min={zs.min():.3f} P1={np.percentile(zs,1):.3f} P5={np.percentile(zs,5):.3f} P10={np.percentile(zs,10):.3f} max={zs.max():.3f}")

# Global ground: SAC on full cloud should find the dominant horizontal plane
# Let's find Z at various Y positions
for y_pos in [-8, -4, 0, 4, 8]:
    near_y = d[abs(d['y']-y_pos)<0.5]
    if len(near_y)>100:
        zs = near_y['z']
        print(f"\nY≈{y_pos}: Z P1={np.percentile(zs,1):.3f} P5={np.percentile(zs,5):.3f} P10={np.percentile(zs,10):.3f}")
