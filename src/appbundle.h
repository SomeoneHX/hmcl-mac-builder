// appbundle.h — macOS .app 应用捆绑包创建声明
#pragma once

#include "config.h"
#include <string>
#include <filesystem>
namespace fs = std::filesystem;

// 创建 macOS .app 包：构建目录结构 → 生成 Info.plist → 复制 JAR/ICNS/启动脚本 → 可选打包 Java
bool CreateAppBundle(const Config& config, const fs::path& jarPath,
                     const fs::path& icnsPath, const fs::path& launcherPath,
                     const std::string& version, const BuildInfo& buildInfo,
                     bool verbose, const fs::path& javaHome = "");
