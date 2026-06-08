# pcd-blind 项目 agent 约束

## 项目结构

```
pcd-blind/                          (本地 & mz 双向同步 via mfr)
├── source/        ← 原始 PCD 点云 (gitignored, 不参与 mfr 同步)
├── scripts/       ← 在 mz 上执行的脚本
├── output/        ← mz 产出
├── archive/       ← 废弃的旧脚本
└── .mainframer/   ← mfr 配置
```

## 核心原则

- **所有开发都在 mz 上。** 脚本写好后 `mfr ./script.sh` 推到 mz 执行，产出自动回传。
- **不要在本地跑分析。** 本地只做代码编辑，不跑 Python 不处理数据。

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
PCL (libpcl-dev, apt 安装)  ← 标靶球检测主力
Python 3                    ← 辅助脚本（如需要）
```

- PCL 版本：1.14 (Ubuntu apt)
- C++ 编译：cmake + make on mz
- 不要引入 open3d、不要用 Mac 上的 Python 做点云处理

## 当前目标

**标靶球检测**：200mm 直径 (r=0.1m) 标靶球，在隧道 PCD 中识别球心坐标。

方向：用 PCL 的 SAC (Sample Consensus) 模块做 RANSAC 球拟合，已知半径约束。

## 文件规范

- `source/`：原始 PCD，只读
- `scripts/`：所有脚本（.sh / .cpp / CMakeLists.txt / .py），通过 mfr 在 mz 执行
- `output/`：产出，按 `{数据集}/{算法}/` 组织
- `archive/`：已废弃
