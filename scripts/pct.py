import struct
with open('source/浙江省计量院一楼实验室.pcd','rb') as f:
    while True:
        l=f.readline().decode().strip()
        if l.startswith('DATA'): break
    buf=f.read()
pts=[struct.unpack('ffff',buf[i:i+16]) for i in range(0,len(buf),16)]
Is=sorted([p[3] for p in pts])
for p in [50,75,80,85,90,95]:
    print(f'P{p}={Is[len(Is)*p//100]:.0f}')
# Also count hi pts near target at diff thresholds
px,py=6.151,5.049
for th in [29,35,50,60]:
    n=sum(1 for x,y,z,i in pts if i>=th and abs(x-px)<0.6 and abs(y-py)<0.6)
    print(f'I>={th}: {n} pts in box')
