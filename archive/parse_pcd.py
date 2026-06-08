#!/usr/bin/env python3
"""解析 PCD v0.7 binary 点云文件，输出基本统计信息"""
import numpy as np
import struct
import sys
import os

def parse_pcd_header(path):
    """读取 PCD 头部，返回 (header_dict, data_offset)"""
    with open(path, 'rb') as f:
        header_lines = []
        for line in f:
            header_lines.append(line.decode('utf-8', errors='replace').rstrip('\n'))
            if line.startswith(b'DATA'):
                break
        data_offset = f.tell()

    header = {}
    for line in header_lines:
        if line.startswith('#') or not line.strip():
            continue
        parts = line.split()
        if len(parts) >= 2:
            key = parts[0]
            val = ' '.join(parts[1:])
            header[key] = val
    return header, data_offset

def read_pcd(path):
    header, offset = parse_pcd_header(path)
    fields = header['FIELDS'].split()
    sizes = [int(s) for s in header['SIZE'].split()]
    types = header['TYPE'].split()
    counts = [int(c) for c in header['COUNT'].split()]
    npoints = int(header['POINTS'])

    type_map = {'F': 'f', 'I': 'i', 'U': 'B'}
    dtype_list = []
    for field, size, t, count in zip(fields, sizes, types, counts):
        fmt = type_map.get(t, 'f')
        if count > 1:
            dtype_list.append((field, f'{fmt}{count}'))
        else:
            dtype_list.append((field, fmt + str(size)))

    dt = np.dtype(dtype_list)
    with open(path, 'rb') as f:
        f.seek(offset)
        data = np.fromfile(f, dtype=dt, count=npoints)

    return header, data, fields

if __name__ == '__main__':
    BASE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    path = sys.argv[1] if len(sys.argv) > 1 else os.path.join(BASE, "source", "绍兴白峰岭隧道.pcd")
    print(f"文件: {os.path.basename(path)}")
    print(f"大小: {os.path.getsize(path)/1024/1024:.1f} MB\n")

    header, data, fields = read_pcd(path)

    # 头部
    print("=== 头部 ===")
    for k, v in header.items():
        print(f"  {k}: {v}")

    # 基本统计
    print(f"\n=== 点云统计 ({len(data):,} 点) ===")
    for f in fields:
        col = data[f]
        print(f"  {f:12s}: min={col.min():.4f}, max={col.max():.4f}, mean={col.mean():.4f}, std={col.std():.4f}")

    # 包围盒
    print(f"\n=== 包围盒 ===")
    print(f"  X: [{data['x'].min():.4f}, {data['x'].max():.4f}]  范围: {data['x'].max()-data['x'].min():.4f}")
    print(f"  Y: [{data['y'].min():.4f}, {data['y'].max():.4f}]  范围: {data['y'].max()-data['y'].min():.4f}")
    print(f"  Z: [{data['z'].min():.4f}, {data['z'].max():.4f}]  范围: {data['z'].max()-data['z'].min():.4f}")

    # intensity 分布
    if 'intensity' in fields:
        intensity = data['intensity']
        print(f"\n=== Intensity 分布 ===")
        for pct in [1, 5, 10, 25, 50, 75, 90, 95, 99]:
            print(f"  P{pct:2d}: {np.percentile(intensity, pct):.4f}")

    # curvature 分布
    if 'curvature' in fields:
        curv = data['curvature']
        print(f"\n=== Curvature 分布 ===")
        print(f"  min={curv.min():.6f}, max={curv.max():.6f}, mean={curv.mean():.6f}")

    # 法向量检查
    if 'normal_x' in fields:
        norms = np.sqrt(data['normal_x']**2 + data['normal_y']**2 + data['normal_z']**2)
        print(f"\n=== 法向量模长 ===")
        print(f"  min={norms.min():.6f}, max={norms.max():.6f}, mean={norms.mean():.6f}")
        print(f"  模长≈1 的比例: {(np.abs(norms-1)<0.01).mean()*100:.1f}%")
