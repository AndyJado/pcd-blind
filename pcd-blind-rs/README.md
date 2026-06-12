# pcd-blind-rs

隧道点云标靶球检测管线（Rust）。

## CLI

```bash
pcd-blind <scene.pcd> <output_dir> [--config config.toml]
```

不指定 `--config` 时使用内置默认参数。

## 管线分层

```
PCD 文件
  │
  ▼
Layer 1: 候选生成（candidates）
  ├── 强度阈值 (I > P80)
  ├── Z 分层切片 (3.0 → -5.0, step 0.2m)
  └── 每层 XY 聚类 + 层间去重合并
  │
  ▼
Layer 2: 迭代定位（recenter）
  ├── 圆柱裁剪 (radius 0.5m, box h=2.3m)
  ├── RANSAC 地面拟合 → 去地面/墙面
  ├── 二分 hi/lo → 迭代修正球心
  └── 输出 match_*.pcd（按需保存）
  │
  ▼
Layer 3: 特征提取（features）
  ├── ratio    球面 / 三脚架强度比
  ├── compact  球面点空间紧密度
  ├── tripod   三脚架三腿均匀度（角度直方图）
  ├── dxy      球心与架心水平偏移
  └── ground   地面点占比
  │
  ▼
Layer 4: 打分过滤（scoring）
  加权打分 → 门控 → 去重 → 排序 → results.csv
```

## 模块

| 文件 | 职责 |
|------|------|
| `pcd.rs` | PCD 文件读写 |
| `candidates.rs` | Z 切片 + XY 聚类 + 合并去重 |
| `recenter.rs` | 迭代球心定位（hi/lo 二分 + 梯度修正） |
| `ground.rs` | RANSAC 地面平面拟合 |
| `features/` | 五项特征提取（ratio / compact / tripod / dxy / ground） |
| `scoring.rs` | 加权打分 + 门控过滤 + 空间去重 |
| `cluster.rs` | DBSCAN-like XY 聚类 |
| `kdtree.rs` | K-d 树（kiddo 封装） |
| `output.rs` | CSV 输出 |
| `render.rs` | 2D 等距投影截图 |
| `types.rs` | Detection / Point / Feature 数据结构 |
| `config.rs` | TOML 配置解析 |

## 配置

TOML 格式，七个段均为可选（不写 = 用默认值）：

```toml
[detect]       # 强度阈值 pct、球直径
[candidates]   # Z 范围、聚类容差、去重参数
[ground]       # 圆柱裁剪半径、RANSAC 参数
[recenter]     # 最大迭代、收敛阈值
[features]     # 五项特征门控阈值
[scoring]      # 权重、去重半径
[output]       # 是否保存 match PCD
```

完整默认值见 `config/default.toml`。

## 依赖

- [kiddo](https://crates.io/crates/kiddo) — K-d 树
- [nalgebra](https://crates.io/crates/nalgebra) — 线性代数
- [clap](https://crates.io/crates/clap) — CLI 参数解析
- [image](https://crates.io/crates/image) — 截图渲染
- [toml](https://crates.io/crates/toml) + [serde](https://crates.io/crates/serde) — 配置解析
