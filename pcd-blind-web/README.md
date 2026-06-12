# 标靶球检测工具

隧道激光点云（PCD）中标靶球自动识别。浏览器操作，单文件部署。

## 使用（预编译二进制）

拿到 `pcd-blind-web` 文件后：

```bash
# macOS / Linux
chmod +x pcd-blind-web
./pcd-blind-web --port 8080

# Windows
pcd-blind-web.exe --port 8080
```

浏览器打开 `http://localhost:8080`。

所有数据（上传的 PCD、检测结果、截图）存放在 `./data/` 目录。

## 操作流程

1. **📁 文件管理** — 上传 PCD 点云文件
2. **⚙ 参数配置** — 调整灵敏度、Z 搜索范围等（核心参数 5 项，高级参数 9 项默认折叠）
3. **▶ 开始检测** — 选择文件后点按钮，等待进度条完成
4. **🔍 检测结果** — 查看 3D 投影截图，用 5 个滑块实时过滤
5. **☑ 确认** — 点击图片卡片上的「确认此靶」，选中的可以导出 CSV

## 过滤条件说明

| 参数 | 含义 | 默认 |
|------|------|------|
| Ratio | 球面/三脚架强度比，越高越可信 | ≥ 5.0 |
| Compact | 球面点空间紧密度（1=完美球） | ≥ 0.88 |
| Dxy | 球心与架心水平偏移，越小越好 | ≤ 0.15m |
| Tripod | 三脚架三腿均匀度 | ≥ 0 |
| Ground | 地面点占比信号 | ≥ 0.05 |

## 从源码构建

需要 Go 1.21+ 和 Rust 工具链。

```bash
# 1. 编译 Rust 管线
cd pcd-blind-rs && cargo build --release

# 2. 编译 Web 前端（embed Rust 二进制）
cd ../pcd-blind-web
make prep    # 下载 HTMX.js + 拷贝 pcd-blind
make build   # 产出单文件 pcd-blind-web

# 3. 运行
./pcd-blind-web --port 8080
```

### 交叉编译

```bash
# Linux（在 Linux 上直接 make 即可）
make build-linux

# Windows
make build-windows
```

Linux/Windows 交叉编译需要先安装 Rust 目标：
```bash
rustup target add x86_64-unknown-linux-gnu
rustup target add x86_64-pc-windows-gnu
```

## 依赖

分发的二进制文件**零外部依赖**。内含：

- Go Web 服务器
- Rust pcd-blind 管线引擎
- HTMX 前端框架
- 全部 HTML/CSS

无需安装 Python、无需联网（HTMX 已内嵌）。
