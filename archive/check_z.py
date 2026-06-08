import struct, numpy as np
path='/home/mz/mainframer/pcd-blind/source/庆元陈家岭隧道1.pcd'
with open(path,'rb') as f:
    for l in f:
        if l.startswith(b'DATA'): break
    off=f.tell()
dt=np.dtype([('x','f4'),('y','f4'),('z','f4'),('i','f4')])
with open(path,'rb') as f:
    f.seek(off); d=np.fromfile(f,dtype=dt)
zs=np.sort(d['z'])
print(f"Z min: {zs[0]:.2f}, max: {zs[-1]:.2f}")
for p in [0.1,0.5,1,2,5,10,25,50,75,90,95,99]:
    print(f"P{p:5.1f}: {zs[int(len(zs)*p/100)]:.2f}")
