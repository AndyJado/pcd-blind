# pcd-blind

隧道激光点云（PCD）中标靶球自动识别管线。

## 环境要求

- Go 1.21+
- Rust 工具链（`cargo`）

## 构建 & 运行

```bash
# 1. 编译 Rust 管线
cd pcd-blind-rs && cargo build --release

# 2. 编译 Web 前端
cd ../pcd-blind-web && make build

# 3. 启动
./pcd-blind-web --port 8080
```

浏览器打开 `http://localhost:8080`。

### 交叉编译

```bash
# Linux（在 Linux 上 native 编译）
make build-linux

# Windows（需要 mingw 工具链）
make build-windows
```

交叉编译需先安装 Rust 目标：

```bash
rustup target add x86_64-unknown-linux-gnu
rustup target add x86_64-pc-windows-gnu
```

## 项目结构

```
pcd-blind/
├── pcd-blind-rs/     ← Rust 管线（主力）
├── pcd-blind-web/    ← Go + HTMX Web 前端
├── scripts/          ← Python 辅助脚本
├── source/           ← 原始 PCD 点云（gitignored）
├── output/           ← 检测产出
├── doc/              ← 文档
└── .mainframer/      ← mfr 远程编译配置
```

## 文档

- [管线架构](doc/PIPELINE.md)
- [操作速查](doc/cheatsheet.md)
- [Rust 管线说明](pcd-blind-rs/README.md)
- [Web 前端说明](pcd-blind-web/README.md)
