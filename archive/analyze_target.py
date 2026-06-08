import numpy as np, struct, sys

def read_pcd_xyzi(path):
    with open(path, 'rb') as f:
        for line in f:
            if line.startswith(b'DATA'): break
        offset = f.tell()
    dt = np.dtype([('x','f4'),('y','f4'),('z','f4'),('intensity','f4')])
    with open(path, 'rb') as f:
        f.seek(offset)
        return np.fromfile(f, dtype=dt)

data = read_pcd_xyzi(sys.argv[1])
ints = data['intensity']
print(f"Points: {len(data)}")
print(f"Intensity: min={ints.min():.0f} max={ints.max():.0f} mean={ints.mean():.1f}")
for p in [1,5,10,25,50,75,90,95,99]:
    print(f"  P{p:2d}: {np.percentile(ints, p):.0f}")

# sphere (upper 40%) vs tripod (lower 60%)
z_cut = data['z'].min() + (data['z'].max()-data['z'].min())*0.6
upper = data[data['z'] >= z_cut]
lower = data[data['z'] < z_cut]
print(f"\nSphere(top): {len(upper)} pts  I {upper['intensity'].min():.0f}-{upper['intensity'].max():.0f} mean={upper['intensity'].mean():.1f}")
print(f"Tripod(low): {len(lower)} pts  I {lower['intensity'].min():.0f}-{lower['intensity'].max():.0f} mean={lower['intensity'].mean():.1f}")

# what fraction of target points would be removed at various thresholds?
for th in [50, 75, 100, 125, 150, 200]:
    removed = (ints >= th).sum()
    print(f"  I>={th}: remove {removed}/{len(data)} ({100*removed/len(data):.1f}%)")
