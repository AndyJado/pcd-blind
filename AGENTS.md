# pcd-blind 项目 agent 约束

## 项目结构

```
pcd-blind/                          (本地 & mz 双向同步 via rsync)
├── source/        ← 原始 PCD 点云 (gitignored)
├── pcd-blind-rs/  ← Rust 管线 (主力)
├── scripts/       ← 旧 C++ 脚本 + Python 辅助
├── output/        ← mz 产出
├── archive/       ← 废弃的旧脚本
└── .mainframer/   ← mfr 配置
```

## 核心原则

- **所有开发都在 mz 上。** Rust 代码写完 rsync 到 mz，`cargo build --release` 编译执行。
- **不要在本地跑分析。** 本地只做代码编辑和 CloudCompare 可视化。
- **管线跑完后自动 kill CloudCompare 并 reopen 最新产出 match PCD。**（键：每次 run 后 `pkill -f CloudCompare; open -a CloudCompare output/xxx/match_*.pcd`）

## 环境

| 机器 | 地址 | 用途 |
|------|------|------|
| mz | mz (Tailscale 100.65.229.110) | 唯一计算节点，PCL 已装 |
| 本地 | Mac | 代码编辑，查看产出图片 |

- `mfr <cmd>` 从项目根目录执行，自动 rsync → mz 执行 → rsync 回传
- mz 路径：`~/mainframer/pcd-blind`（mfr）或 `~/phd/pcd-blind`（symlink）
- `source/*.pcd` 受 localignore/remoteignore 保护，不参与 mfr 同步
- 传大文件用 `rsync`，不用 `scp`

## 技术栈

```
Rust (pcd-blind-rs)  ← 标靶球检测主力管线
  - kiddo (kd-tree)
  - nalgebra (3D 数学)
  - 自实现: PCD 读写, RANSAC 平面, Euclidean 聚类
PCL 1.14 (libpcl-dev) ← 已弃用，仅旧代码参考
Python 3              ← 辅助脚本（渲染/汇报）
```

- mz 上 Rust: `~/.cargo/bin/cargo` (1.89)
- 编译: `cd pcd-blind-rs && ~/.cargo/bin/cargo build --release`
- 不要引入 open3d

## 管线架构

### 物理模型

```
                   ╭──────╮
                  ╱  球面   ╲    hi：反光球面（高强度）
                 │    ○     │
                  ╲        ╱
                   ╰──────╯
                     ╱│╲         lo：三脚架 3 腿（低强度）
                    ╱ │ ╲
    ────────────┴─────┴─────┴───  地面（Z-base 去除）
```

标靶球 r=0.1m，固定在三角架上，架在地面。球面反光涂层产生高强度回波。

### 三层管线

```
┌─ 层1: 候选定位 ──────────────────────────────────────┐
│  P80高通 → Z切片(3→-5m,0.2m) → XY投影 → 2D聚类 → 去重  │
│  → for each: 圆柱(r=0.5m)裁剪 → Z-base地面去除         │
│  → dxy recenter loop (max 3 iters)                    │
│     强度k-means分裂 → lo/hi质心 → dxy                  │
│     if dxy>0.15: 取中点修正cx,cy → 重裁剪              │
├────────────────────────────────────────────────────────┤
│ 层2: 特征计算 (纯函数)                                  │
│  ratio   = hi_mean / (lo_mean + 0.1)   ← 强度双峰比    │
│  tripod  = lo XY角分布3峰质量         ← 三脚架验证      │
│  compact = hi最高点下0.24m窗 3D方差    ← 球面紧密      │
│  dxy     = hi/lo质心XY偏移             ← 定位精度      │
│  ground  = 删除点数/原始点数           ← 地面存在       │
├────────────────────────────────────────────────────────┤
│ 层3: 评分决策                                          │
│  gate: ratio≥5.0, compact≥0.88, dxy≤0.15, ground≥0.05 │
│  score = 4·tripod + 1·ratio + 2·compact - 3·dxy + 3·ground │
│  dedup (XY<0.5m + Z<1.0m) → 按score降序               │
└────────────────────────────────────────────────────────┘
```

### 核心设计决策

| 决策 | 为什么 |
|------|--------|
| Z-base 地面去除 | 取圆柱内最低 10% Z 点，拟合水平面删除。外环 RANSAC 墙面而非地面 |
| dxy recenter | hi/lo 质心偏移驱动候选重定位，收敛后 dxy 存入特征层 |
| tripod 不计入 gate | 数据质量不稳定（灰尘、遮挡），只增强排序 |
| 2D 角分布 tripod | 3D 聚类因 lo 点密度不够而失效；XY 投影天然隔离腿方向 |

### 参数 (config/default.toml)

| 参数 | 默认值 | 作用 |
|------|--------|------|
| detect.pct | 80 | 强度高通百分比 |
| candidates.z_start/step/end | 3.0/0.2/-5.0 | Z 切片范围 |
| candidates.cluster_tolerance | 0.15 | 2D 聚类半径 |
| ground.cyl_radius | 0.5 | 圆柱裁剪半径 |
| ground.ground_z_pct | 0.10 | 地面 Z 百分位 |
| ground.wall_nz_max | 0.3 | 墙面法向量 Z 分量阈值 |
| recenter.max_iters | 3 | 最大迭代次数 |
| recenter.dxy_threshold | 0.15 | 收敛阈值 |
| features.ratio_min | 5.0 | ratio 门控 |
| features.compact_min | 0.88 | compact 门控 |
| features.compact_z_window | 0.24 | hi Z-crop 窗口 |
| features.dxy_max_residual | 0.15 | dxy 门控 |
| features.ground_fraction_min | 0.05 | ground 门控 |
| features.tripod_enabled | true | tripod 开关 |
| scoring.*_weight | 4/1/2/-3/3 | 各特征权重 |
| scoring.dedup_radius/z | 0.5/1.0 | 去重半径 |

### 运行

```bash
# 编译
rsync -avz pcd-blind-rs/ mz:~/phd/pcd-blind/pcd-blind-rs/
ssh mz "cd ~/phd/pcd-blind/pcd-blind-rs && ~/.cargo/bin/cargo build --release"

# 执行
ssh mz "cd ~/phd/pcd-blind && ./pcd-blind-rs/target/release/pcd-blind \
  -c pcd-blind-rs/config/default.toml \
  source/庆元陈家岭隧道1.pcd output/rust_qy1"

# 查看
rsync -avz mz:~/phd/pcd-blind/output/rust_qy1/ output/rust_qy1/
pkill -f CloudCompare; open -a CloudCompare output/rust_qy1/match_*.pcd
```

## 文件规范

- `source/`：原始 PCD，只读
- `scripts/`：所有脚本（.sh / .cpp / CMakeLists.txt / .py），通过 mfr 在 mz 执行
- `output/`：产出，按 `{数据集}/{算法}/` 组织
- `archive/`：已废弃
