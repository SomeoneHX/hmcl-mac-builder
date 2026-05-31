// config.h — 构建元信息与命令行配置定义
#pragma once

#include <string>
#include <filesystem>
namespace fs = std::filesystem;

// BuildInfo: 记录构建时的环境与工具版本元信息，最终嵌入启动脚本和 Info.plist
struct BuildInfo {
    std::string builderVersion;   // 本工具自身版本号
    std::string builderName;      // 本工具名称
    std::string builderRepo;      // 本工具仓库地址
    std::string buildDate;        // 构建日期 (YYYY-MM-DD)
    std::string buildTime;        // 构建时间 (HH:MM:SS)
    std::string buildArgs;        // 构建时使用的命令参数
    std::string macOSVersion;     // macOS 系统版本
    std::string macOSBuild;       // macOS 构建号
    std::string architecture;     // 硬件架构 (如 arm64, x86_64)
    std::string compilerVersion;  // C++ 编译器版本
    std::string cmakeVersion;     // CMake 版本
    std::string xcodeVersion;     // Xcode 版本
    std::string javaVersion;      // 打包的 Java 版本
    std::string javaHome;         // 打包的 Java 路径
    std::string createDmgVersion; // create-dmg 工具版本
};

// Config: 命令行参数解析后的全局配置
struct Config {
    fs::path outputDir = fs::current_path() / "output"; // 输出目录（默认 ./output）
    std::string appName = "HMCL";                       // 应用名称（默认 HMCL）
    std::string jarPath;                                // 本地 JAR 路径（为空则从 GitHub 下载）
    std::string tag;                                    // HMCL 版本标签（为空则获取最新版）
    std::string proxyUrl;                               // GitHub 下载代理地址
    std::string lang;                                   // 语言 (en / zh)
    bool bundleJava = false;                            // 是否打包 Java 运行时到 .app
    std::string javaPath;                               // 用户指定的 Java 路径
    bool noDmg = false;                                 // 跳过 DMG 创建
    bool skipIcon = false;                              // 跳过图标处理
    bool clean = false;                                 // 仅清理输出目录
    bool keepTemp = false;                              // 保留临时文件
    bool verbose = false;                               // 启用详细日志输出
};

// 解析命令行参数，填充 Config；失败返回 false
bool ParseArgs(int argc, char* argv[], Config& config);
// 打印帮助信息
void PrintUsage(const char* programName);
// 打印版本信息
void PrintVersion();
