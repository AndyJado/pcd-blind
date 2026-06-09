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

## 当前目标

**标靶球检测**：200mm 直径 (r=0.1m) 标靶球，在隧道 PCD 中识别球心坐标。

管线架构：Z-slice → XY 聚类 → 圆柱裁剪 → Z-base 地面去除 → dxy recenter →
特征提取 (ratio/tripod/compact/dxy) → 评分排序

## 文件规范

- `source/`：原始 PCD，只读
- `scripts/`：所有脚本（.sh / .cpp / CMakeLists.txt / .py），通过 mfr 在 mz 执行
- `output/`：产出，按 `{数据集}/{算法}/` 组织
- `archive/`：已废弃
