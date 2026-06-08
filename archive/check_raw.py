import struct, sys
path = sys.argv[1]
with open(path, 'rb') as f:
    raw = f.read()
idx = raw.find(b'DATA binary')
data_start = idx + len(b'DATA binary') + 1
actual_data = len(raw) - data_start
expected = 10851192 * 32
print("Header end byte:", data_start)
print("File size:", len(raw))
print("Data bytes:", actual_data, "expected:", expected)
print("Match:", actual_data == expected)
# first 4 floats
f = struct.unpack('4f', raw[data_start:data_start+16])
print("First 4 floats:", f)
# try field 3 (intensity) of first few points
for i in range(3):
    off = data_start + i*32
    x,y,z,intensity = struct.unpack('4f', raw[off:off+16])
    print("  pt", i, ":", x, y, z, intensity)
