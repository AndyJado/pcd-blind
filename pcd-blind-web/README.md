# pcd-blind-web

标靶球检测 Web 前端。Go + HTMX，浏览器操作。

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

## 代码结构

```
cmd/server/
├── main.go           ← 入口，embed Rust 二进制 + 模板 + HTMX
├── bin/              ← make prep 拷贝的 pcd-blind 二进制（gitignored）
└── web/
    ├── templates/    ← Go html/template（base / upload / params / results / history / runs）
    └── static/       ← HTMX.js

internal/
├── handler/          ← HTTP 路由 & HTMX 响应
│   ├── routes.go     ← 路由注册
│   ├── upload.go     ← 文件上传
│   ├── params.go     ← 参数配置页
│   ├── runs.go       ← 检测执行（调用 Rust 子进程）
│   ├── results.go    ← 结果过滤 & CSV 导出
│   └── render.go     ← 3D 截图渲染
├── engine/
│   ├── runner.go     ← Rust 子进程调用封装
│   └── scorer.go     ← 打分 & 门控逻辑（Go 侧复现）
├── store/
│   └── store.go      ← JSON 文件持久化（runs / uploads）
└── config/
    └── config.go     ← 配置结构
```

编译时通过 `//go:embed` 将 Rust 二进制、HTML 模板、HTMX.js 全部打包为单文件。

## 技术栈

- Go 标准库 `net/http` + [chi](https://github.com/go-chi/chi) 路由
- [HTMX](https://htmx.org) 前端交互
- Go `html/template` 服务端渲染
- Rust 管线作为子进程调用
