# HMCL macOS Builder

将 [HMCL (Hello Minecraft! Launcher)](https://github.com/HMCL-dev/HMCL) 打包为原生 macOS `.app` 应用捆绑包，并可选生成 `.dmg` 磁盘映像的命令行工具。

## 功能特性

- 自动从 GitHub Releases 获取最新 HMCL JAR 包
- 支持指定本地 JAR 文件（跳过下载）
- 自动下载并转换 HMCL 图标为 macOS `.icns` 格式
- 生成启动脚本（`launch.sh`），自动定位当前目录的 JAR 并调用 Java 运行
- 组装完整的 `.app` 目录结构（`Info.plist`、`Resources`、`MacOS`）
- 可选调用 `create-dmg` 生成 `.dmg` 磁盘映像
- 支持自定义应用名称、输出目录、版本标签

## 环境要求

### 构建依赖

| 依赖 | 说明 |
|---|---|
| CMake >= 3.15 | 构建系统 |
| C++17 编译器 | Apple Clang（Xcode Command Line Tools） |
| libcurl | HTTP 下载支持（`brew install curl` 或通过 Xcode CLT 自带） |
| nlohmann/json v3.11.3 | JSON 解析（CMake 构建时自动下载） |

### 运行依赖

| 依赖 | 说明 | 安装方式 |
|---|---|---|
| `sips` + `iconutil` | macOS 内置，用于图标转换 | 无需安装 |
| `create-dmg` | 可选，创建 `.dmg` 映像 | `brew install create-dmg` |
| ImageMagick | 可选，图标转换的备用方案 | `brew install imagemagick` |
| Java Runtime | 运行 HMCL 所必需 | 从 [Adoptium](https://adoptium.net) 或 [Oracle](https://java.com) 安装 |

## 构建

```bash
# 配置（Release 模式）
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 编译
cmake --build build

# 编译产物位于 build/hmcl-mac-builder
```

## 使用方法

```bash
# 查看帮助
./build/hmcl-mac-builder --help

# 基本用法：下载最新 HMCL，生成 .app + .dmg 到 ./output/
./build/hmcl-mac-builder

# 使用本地 JAR，跳过 DMG 生成，保留临时文件
./build/hmcl-mac-builder --jar /path/to/HMCL.jar --no-dmg --keep-temp

# 指定应用名称和输出目录
./build/hmcl-mac-builder --app-name "MyHMCL" --output ~/Desktop

# 指定 GitHub Release 标签
./build/hmcl-mac-builder --tag v3.5.3

# 跳过图标处理
./build/hmcl-mac-builder --skip-icon

# 使用 GitHub 下载代理（仅对文件下载生效，API 请求不使用）
./build/hmcl-mac-builder --proxy https://v4.gh-proxy.org/

# 清理之前构建的文件
./build/hmcl-mac-builder --clean
```

## 命令行选项

| 选项 | 说明 | 默认值 |
|---|---|---|
| `-h, --help` | 显示帮助信息 | - |
| `-v, --version` | 显示版本号 | - |
| `-o, --output DIR` | 输出目录 | `./output` |
| `--app-name NAME` | 应用名称 | `HMCL` |
| `--jar PATH` | 指定本地 JAR 文件（跳过 GitHub 下载） | 从 GitHub 下载 |
| `--tag VERSION` | GitHub Release 标签 | 最新稳定版 |
| `--no-dmg` | 仅生成 `.app`，跳过 `.dmg` 创建 | 生成 `.dmg` |
| `--skip-icon` | 跳过图标处理 | 处理图标 |
| `--clean` | 清理之前的构建文件 | 不清除 |
| `--proxy URL` | GitHub 下载代理前缀（仅对文件下载生效，API 请求不使用） | 不使用 |
| `--keep-temp` | 构建完成后保留临时文件 | 自动删除 |
| `--verbose` | 启用详细日志输出 | 仅显示概要 |

## 工作原理

```
解析参数 → 获取 Release 信息 → 下载 JAR → 处理图标 → 生成启动脚本 →
组装 .app（Info.plist + JAR + ICNS + launch.sh）→ 创建 .dmg → 清理临时文件
```

每一步均可独立观察：启用 `--verbose` 和 `--keep-temp` 可查看完整过程。

## 项目结构

```
hmcl-mac-builder/
├── CMakeLists.txt          # CMake 构建配置
├── .gitignore              # Git 忽略规则
└── src/
    ├── main.cpp            # 入口，编排构建流水线
    ├── config.h / .cpp     # Config 结构体、CLI 参数解析
    ├── utils.h / .cpp      # 工具函数：日志、文件操作、临时目录
    ├── network.h / .cpp    # HTTP 下载 + GitHub API Release 查询
    ├── icon.h / .cpp       # 图标下载与 ICNS 格式转换
    ├── launcher.h / .cpp   # launch.sh 启动脚本生成
    ├── appbundle.h / .cpp  # .app 捆绑包组装（Info.plist 等）
    └── dmg.h / .cpp        # 调用 create-dmg 生成磁盘映像
```

## 许可

此项目目前未声明许可证。
