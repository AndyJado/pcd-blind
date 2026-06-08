import struct
px, py, pz = 6.151, 5.049, -0.597
dx, dy, dz_up, dz_dn = 0.6, 0.6, 0.3, 2.0
with open('source/浙江省计量院一楼实验室.pcd','rb') as f:
    while True:
        l = f.readline().decode().strip()
        if l.startswith('DATA'): break
    buf = f.read()
pts = [struct.unpack('ffff', buf[i:i+16]) for i in range(0, len(buf), 16)]

box = [(x,y,z,i) for x,y,z,i in pts if
       px-dx<=x<=px+dx and py-dy<=y<=py+dy and pz-dz_dn<=z<=pz+dz_up]
print(f"Box: {len(box)} pts")

# k-means to find hi/lo split
box_i = sorted([p[3] for p in box])
n = len(box)
best_var = 1e9; sp = n//10
for q in range(n//10, n*9//10):
    lo_m = sum(box_i[:q])/q; hi_m = sum(box_i[q:])/(n-q)
    v = sum((v-lo_m)**2 for v in box_i[:q]) + sum((v-hi_m)**2 for v in box_i[q:])
    if v < best_var: best_var = v; sp = q
split_I = box_i[sp]

# Save as PointXYZI PCD with colored intensity
header = f"""# .PCD v0.7 - Point Cloud Data file format
VERSION 0.7
FIELDS x y z intensity
SIZE 4 4 4 4
TYPE F F F F
COUNT 1 1 1 1
WIDTH {n}
HEIGHT 1
VIEWPOINT 0 0 0 1 0 0 0
POINTS {n}
DATA binary
"""
out = []
for x,y,z,i in box:
    color = 255 if i >= split_I else 50
    out.append(struct.pack('ffff', x, y, z, float(color)))

with open('output/计量院_miss_box.pcd', 'wb') as f:
    f.write(header.encode())
    f.write(b''.join(out))
print(f"split_I={split_I:.0f}  saved output/计量院_miss_box.pcd")
