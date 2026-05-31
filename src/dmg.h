// dmg.h — DMG 磁盘镜像创建声明
#pragma once

#include "config.h"
#include <string>

// 使用 create-dmg 工具将 .app 打包为 macOS DMG 安装包
bool CreateDMG(const Config& config, const std::string& version, bool verbose, const std::string& javaVersion = "");
