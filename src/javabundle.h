// javabundle.h — Java 运行时检测与打包声明
#pragma once

#include <string>
#include <filesystem>
namespace fs = std::filesystem;

// JavaInfo: Java 运行时检测结果
struct JavaInfo {
    fs::path javaHome;       // JDK 根目录
    std::string version;     // 版本字符串（如 "17.0.10"）
    std::string architecture; // 架构（如 "aarch64", "x86_64"）
    bool valid = false;      // 是否找到有效的 Java 17+
};

// 自动检测本机 Java：依次检查用户指定路径 → JAVA_HOME → java_home → /Library/Java → /usr/bin/java
JavaInfo FindJava(const std::string& userPath = "");
// 将 Java 运行时复制到 .app 中的指定目录（跳过 man/include/jmods 等非运行所需文件）
bool BundleJava(const fs::path& javaHome, const fs::path& destDir, bool verbose);
