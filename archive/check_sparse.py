import struct, sys
path = sys.argv[1]
with open(path, 'rb') as f:
    raw = f.read()
idx = raw.find(b'DATA binary')
data_start = idx + len(b'DATA binary') + 1

# check points at various positions in the file
for pt_idx in [0, 100000, 1000000, 5000000, 10000000]:
    off = data_start + pt_idx * 32
    if off + 32 > len(raw): break
    x,y,z,i,*_ = struct.unpack('8f', raw[off:off+32])
    print(f"pt {pt_idx}: ({x:.2f}, {y:.2f}, {z:.2f}) I={i:.0f}")
