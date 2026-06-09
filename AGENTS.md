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

- **mz 上编译执行。** `rsync` 推送代码 → `ssh mz` 编译运行 → `rsync` 回传产出。
- **管线跑完后 reopen CloudCompare。** `pkill -f CloudCompare; open -a CloudCompare output/xxx/match_*.pcd`

## 环境

| 机器 | 地址 | 用途 |
|------|------|------|
| mz | mz (Tailscale 100.65.229.110) | 唯一计算节点 |
| 本地 | Mac | 代码编辑，查看产出 |

- mz 路径：`~/mainframer/pcd-blind`（mfr）或 `~/phd/pcd-blind`（symlink）
- `source/*.pcd` 受 localignore/remoteignore 保护，不参与 mfr 同步
- 传大文件用 `rsync`，不用 `scp`

## 技术栈

```
Rust (pcd-blind-rs)     ← 标靶球检测主力管线
Python 3                ← 辅助脚本（渲染/汇报）
```

- mz 上 Rust: `~/.cargo/bin/cargo` (1.89)
- 禁止引入 open3d

## 文件规范

- `source/`：原始 PCD，只读
- `scripts/`：旧 C++/Python，已弃用保留参考
- `output/`：产出，按 `{数据集}/{算法}/` 组织
- `archive/`：已废弃
- `PIPELINE.md`：管线架构文档（非约束）
