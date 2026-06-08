import struct, numpy as np, sys

def pca_features(path, label):
    with open(path, 'rb') as f:
        for l in f:
            if l.startswith(b'DATA'): break
        off = f.tell()
    n_fields = 8 if b'normal' in l else 4
    dt = np.dtype([('x','f4'),('y','f4'),('z','f4'),('i','f4')])
    with open(path, 'rb') as f:
        f.seek(off)
        d = np.fromfile(f, dtype=dt)
    
    pts = np.column_stack([d['x'], d['y'], d['z']])
    mn, mx = pts.min(axis=0), pts.max(axis=0)
    H, W, D = mx[2]-mn[2], mx[0]-mn[0], mx[1]-mn[1]
    
    centered = pts - pts.mean(axis=0)
    cov = centered.T @ centered / len(pts)
    ev, evc = np.linalg.eigh(cov)
    ev = ev[::-1]; evc = evc[:, ::-1]
    
    l1l2 = ev[0]/ev[1] if ev[1]>0 else 0
    l2l3 = ev[1]/ev[2] if ev[2]>0 else 0
    dir1 = evc[:, 0]
    if dir1[2] < 0: dir1 = -dir1
    vert = abs(dir1[2])
    zoff = (pts.mean(axis=0)[2] - mn[2]) / (H + 0.001)
    
    print(f"{label:8s} {len(d):5d}  {H:.2f}  {W:.2f}  {D:.2f}  {ev[0]:.3f} {ev[1]:.3f} {ev[2]:.3f}  {l1l2:5.1f} {l2l3:5.1f}  {vert:.2f} {zoff:.2f}")

# BF targets
for t in ['t1','t2','t3','t4']:
    pca_features(f"/home/mz/mainframer/pcd-blind/output/targets/{t}_noground.pcd", f"BF_{t}")
print()
# QL3 targets
pca_features("/home/mz/mainframer/pcd-blind/output/targets_ql3/t1_noground.pcd", "QL3_T1")
pca_features("/home/mz/mainframer/pcd-blind/output/targets_ql3/t2_noground.pcd", "QL3_T2")
