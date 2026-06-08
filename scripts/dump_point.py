import struct
px, py = 6.151, 5.049
with open('source/浙江省计量院一楼实验室.pcd','rb') as f:
    while True:
        l = f.readline().decode().strip()
        if l.startswith('DATA'): break
    buf = f.read()
pts = [struct.unpack('ffff', buf[i:i+16]) for i in range(0, len(buf), 16)]

# Hi pts near target
hi = [(x,y,z,i) for x,y,z,i in pts if i>=29]
print(f"Total hi (I>=29): {len(hi)}")

# Count hi pts in box around target
dx, dy = 0.6, 0.6
box_hi = [(x,y,z,i) for x,y,z,i in hi if px-dx<=x<=px+dx and py-dy<=y<=py+dy]
print(f"Box hi pts: {len(box_hi)}")

# How many hi pts in a 0.3m radius (dense sphere core)?
r = 0.3
core = [(x,y,z,i) for x,y,z,i in hi if (x-px)**2+(y-py)**2<r*r]
print(f"Hi pts within {r}m radius: {len(core)}")

# What's the nearest other hi concentration?
# Check larger region
R = 5.0
region = [(x,y,z,i) for x,y,z,i in hi if px-R<=x<=px+R and py-R<=y<=py+R]
print(f"Hi pts within {R}m: {len(region)}")
if region:
    xs=[p[0] for p in region]; ys=[p[1] for p in region]
    print(f"  X: {min(xs):.1f}-{max(xs):.1f}  Y: {min(ys):.1f}-{max(ys):.1f}")
