import struct
px, py, pz = 69.921, 3.823, -2.402
with open('source/庆元陈家岭隧道1.pcd','rb') as f:
    while True:
        l = f.readline().decode().strip()
        if l.startswith('DATA'): break
    buf = f.read()
pts = [struct.unpack('ffff', buf[i:i+16]) for i in range(0, len(buf), 16)]

# Global I stats
Is = sorted([p[3] for p in pts])
print(f"Global I: P50={Is[len(Is)//2]:.0f} P80={Is[len(Is)*80//100]:.0f} P90={Is[len(Is)*90//100]:.0f}")
P80 = Is[len(Is)*80//100]

# Box around target
dx, dy, dz_up, dz_dn = 0.6, 0.6, 0.3, 2.0
box = [(x,y,z,i) for x,y,z,i in pts if px-dx<=x<=px+dx and py-dy<=y<=py+dy and pz-dz_dn<=z<=pz+dz_up]
print(f"Box: {len(box)} pts")

hi = [(x,y,z,i) for x,y,z,i in box if i>=P80]
print(f"I>={P80} (P80): {len(hi)} pts")

if len(box) > 0:
    box_i = sorted([p[3] for p in box])
    n = len(box_i)
    # k-means
    bv=1e9; sp=n//10
    for q in range(n//10, n*9//10):
        lo_m=sum(box_i[:q])/q; hi_m=sum(box_i[q:])/(n-q)
        v=sum((v-lo_m)**2 for v in box_i[:q])+sum((v-hi_m)**2 for v in box_i[q:])
        if v<bv: bv=v; sp=q
    lo_m = sum(box_i[:sp])/sp
    hi_m = sum(box_i[sp:])/(n-sp)
    print(f"k-means: split_I={box_i[sp]:.0f} lo={sp}({lo_m:.1f}) hi={n-sp}({hi_m:.1f}) ratio={hi_m/(lo_m+0.1):.1f}")

# Nearest point to picked
best = min(pts, key=lambda p: (p[0]-px)**2+(p[1]-py)**2+(p[2]-pz)**2)
print(f"Nearest to picked: ({best[0]:.3f}, {best[1]:.3f}, {best[2]:.3f}) I={best[3]:.0f}")
