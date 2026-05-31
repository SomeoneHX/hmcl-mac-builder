// icon.h — macOS ICNS 图标生成声明
#pragma once

#include <string>
#include <filesystem>
namespace fs = std::filesystem;

// 完整图标处理流程：下载 HMCL 图标 → 转 PNG → 生成 iconset → 输出 ICNS
bool ProcessIcon(const fs::path& outputPath, bool verbose, const std::string& proxyUrl = "");
