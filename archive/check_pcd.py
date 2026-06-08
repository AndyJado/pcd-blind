import struct, numpy as np, sys
path = sys.argv[1]
with open(path, 'rb') as f:
    for line in f:
        if line.startswith(b'DATA'): break
    off = f.tell()
dt = np.dtype([('x','f4'),('y','f4'),('z','f4'),('i','f4'),('nx','f4'),('ny','f4'),('nz','f4'),('c','f4')])
with open(path, 'rb') as f:
    f.seek(off)
    d = np.fromfile(f, dtype=dt)
print(f"Total: {len(d):,}")
print(f"X: {d['x'].min():.2f} - {d['x'].max():.2f}")
print(f"Y: {d['y'].min():.2f} - {d['y'].max():.2f}")
print(f"Z: {d['z'].min():.2f} - {d['z'].max():.2f}")

cx, cy = float(sys.argv[2]), float(sys.argv[3])
near = d[(abs(d['x']-cx)<1) & (abs(d['y']-cy)<1)]
print(f"\nNear ({cx},{cy}): {len(near)} pts")
if len(near) > 0:
    print(f"  X: {near['x'].min():.2f} - {near['x'].max():.2f}")
    print(f"  Y: {near['y'].min():.2f} - {near['y'].max():.2f}")
    print(f"  Z: {near['z'].min():.2f} - {near['z'].max():.2f}")
    print(f"  intensity: {near['i'].min():.0f} - {near['i'].max():.0f}")
