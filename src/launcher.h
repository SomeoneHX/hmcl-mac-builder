// launcher.h — macOS .app 启动脚本生成声明
#pragma once

#include "config.h"
#include <string>
#include <filesystem>
namespace fs = std::filesystem;

// 生成 launch.sh 脚本：包含构建元信息注释头、Java 检测、JAR 启动逻辑
bool GenerateLauncherScript(const fs::path& outputPath, const std::string& appName,
                            const std::string& version, const BuildInfo& buildInfo,
                            bool bundleJava = false);
