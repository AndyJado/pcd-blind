import struct
path = 'source/拓普康设备高精度点云.pcd'
with open(path, 'rb') as f:
    while True:
        l = f.readline().decode().strip()
        if l.startswith('DATA'): break
    buf = f.read()
pts = [struct.unpack('ffff', buf[i:i+16]) for i in range(0, len(buf), 16)]
xs, ys, zs, Is = [p[0] for p in pts], [p[1] for p in pts], [p[2] for p in pts], [p[3] for p in pts]
Is.sort()
area = (max(xs)-min(xs)) * (max(ys)-min(ys))
vol = area * (max(zs)-min(zs))
print(f"拓普康: {len(pts)/1e6:.1f}M pts  XY={max(xs)-min(xs):.0f}x{max(ys)-min(ys):.0f}m  area={area:.0f}m2  vol={vol:.0f}m3")
print(f"  density: {len(pts)/area:.0f} pts/m2  {len(pts)/vol:.0f} pts/m3" if vol>0 else "")
print(f"  I: P10={Is[len(Is)//10]:.0f} P50={Is[len(Is)//2]:.0f} P80={Is[len(Is)*80//100]:.0f} P90={Is[len(Is)*90//100]:.0f} max={Is[-1]:.0f}")
print(f"  Z: {min(zs):.1f} to {max(zs):.1f}")
