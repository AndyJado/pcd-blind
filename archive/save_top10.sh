#!/bin/bash
set -e
python3 << 'PY'
import struct, numpy as np, csv, os

clusters = []
with open("/home/mz/mainframer/pcd-blind/output/庆元一_final/all_clusters.csv") as f:
    for row in csv.DictReader(f):
        clusters.append((float(row['score']), float(row['cx']), float(row['cy']), float(row['cz'])))
clusters.sort(key=lambda x: -x[0])

path = "/home/mz/mainframer/pcd-blind/output/庆元一_final/preprocessed.pcd"
with open(path,'rb') as f:
    for l in f:
        if l.startswith(b'DATA'): break
    off=f.tell()
dt=np.dtype([('x','f4'),('y','f4'),('z','f4'),('i','f4')])
with open(path,'rb') as f:
    f.seek(off); d=np.fromfile(f,dtype=dt)

for i,(s,cx,cy,cz) in enumerate(clusters[:10]):
    R=1.5
    m=(abs(d['x']-cx)<R)&(abs(d['y']-cy)<R)&(abs(d['z']-cz)<R)
    pts=d[m]
    out=f"/home/mz/mainframer/pcd-blind/output/庆元一_final/clusters/r{i:02d}_s{s:.1f}.pcd"
    hdr=f"# .PCD v0.7\nVERSION 0.7\nFIELDS x y z intensity\nSIZE 4 4 4 4\nTYPE F F F F\nCOUNT 1 1 1 1\nWIDTH {len(pts)}\nHEIGHT 1\nVIEWPOINT 0 0 0 1 0 0 0\nPOINTS {len(pts)}\nDATA binary\n"
    with open(out,'wb') as f:
        f.write(hdr.encode())
        for p in pts: f.write(struct.pack('4f',p['x'],p['y'],p['z'],p['i']))
    print(f"{out} ({len(pts)} pts)")
PY
