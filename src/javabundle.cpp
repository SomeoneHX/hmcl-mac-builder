// javabundle.cpp — 自动检测本机 JDK 并将 Java 运行时打包到 .app 中
#include "javabundle.h"
#include "utils.h"
#include "i18n.h"
#include <iostream>
#include <regex>
#include <algorithm>
#include <cstdlib>
#include <vector>
#include <set>

// 运行 java -version 并解析输出中的版本号
static bool GetJavaVersion(const fs::path& javaBinary, std::string& version) {
    std::string cmd = "\"" + javaBinary.string() + "\" -version 2>&1";
    std::string output;
    if (!RunCommandCapture(cmd, output)) {
        return false;
    }
    // 优先匹配带构建号的版本格式，如 "17.0.10+9"
    std::regex versionRegex("\"(\\d+\\.\\d+\\.\\d+[_+][^\\s\"]*|\\d+\\.\\d+\\.\\d+|\\d+)");
    std::smatch match;
    if (std::regex_search(output, match, versionRegex)) {
        version = match[1];
        return true;
    }
    // 回退匹配主版本号格式
    versionRegex = std::regex("(\\d+)\\.(\\d+)\\.(\\d+)");
    if (std::regex_search(output, match, versionRegex)) {
        version = match[0];
        return true;
    }
    return false;
}

// 从版本字符串中提取主版本号（如 "17.0.10" → 17）
static int ParseMajorVersion(const std::string& version) {
    size_t dot = version.find('.');
    if (dot == std::string::npos) {
        return std::atoi(version.c_str());
    }
    return std::atoi(version.substr(0, dot).c_str());
}

// 验证指定路径是否为有效的 Java Home（存在 bin/java 且可获取版本）
static bool IsValidJavaHome(const fs::path& javaHome, std::string& version) {
    fs::path javaBin = javaHome / "bin" / "java";
    if (!fs::exists(javaBin)) {
        return false;
    }
    return GetJavaVersion(javaBin, version);
}

// 检查 Java 主版本是否满足最低要求（>= minMajor）
static bool IsJavaVersionSatisfies(const std::string& version, int minMajor) {
    int major = ParseMajorVersion(version);
    return major >= minMajor;
}

// 检查一个候选路径是否为有效的 Java Home（支持标准 JDK 目录结构和 macOS .jdk 包结构）
static JavaInfo CheckJavaPath(const fs::path& path) {
    JavaInfo info;
    fs::path javaHome;
    fs::path candidate = path;
    if (fs::exists(candidate / "bin" / "java")) {
        javaHome = candidate;
    } else if (fs::exists(candidate / "Contents" / "Home" / "bin" / "java")) {
        // macOS .jdk 包内的标准布局
        javaHome = candidate / "Contents" / "Home";
    } else {
        return info;
    }
    std::string version;
    if (IsValidJavaHome(javaHome, version)) {
        info.javaHome = javaHome;
        info.version = version;
        info.valid = true;
    }
    return info;
}

// 扫描 /Library/Java/JavaVirtualMachines 目录下的 JDK 安装
static void ScanJavaVirtualMachines(const fs::path& baseDir, std::vector<JavaInfo>& results) {
    fs::path vmDir = baseDir / "JavaVirtualMachines";
    if (!fs::exists(vmDir)) return;
    try {
        for (auto& entry : fs::directory_iterator(vmDir)) {
            if (!entry.is_directory()) continue;
            JavaInfo info = CheckJavaPath(entry.path());
            if (info.valid) {
                results.push_back(info);
            }
        }
    } catch (const fs::filesystem_error&) {}
}

// 多级回退策略检测本机 Java 17+：
// 用户指定路径 → JAVA_HOME → /usr/libexec/java_home → /Library/Java → /usr/bin/java
JavaInfo FindJava(const std::string& userPath) {
    // 1. 用户通过 --java-path 明确指定的路径
    if (!userPath.empty()) {
        LOG_VERBOSE("Checking user-specified Java path: " << userPath, true);
        JavaInfo info = CheckJavaPath(userPath);
        if (info.valid) {
            return info;
        }
        LOG_WARNING("User-specified Java path is invalid: {}", userPath);
    }

    // 2. $JAVA_HOME 环境变量
    const char* javaHomeEnv = std::getenv("JAVA_HOME");
    if (javaHomeEnv && javaHomeEnv[0] != '\0') {
        LOG_VERBOSE("Checking JAVA_HOME: " << javaHomeEnv, true);
        JavaInfo info = CheckJavaPath(javaHomeEnv);
        if (info.valid) {
            return info;
        }
    }

    // 3. macOS 的 /usr/libexec/java_home 命令
    {
        std::string output;
        if (RunCommandCapture("/usr/libexec/java_home 2>/dev/null", output)) {
            std::string path = output;
            path.erase(path.find_last_not_of(" \n\r\t") + 1);
            if (!path.empty()) {
                LOG_VERBOSE("Checking /usr/libexec/java_home: " << path, true);
                JavaInfo info = CheckJavaPath(path);
                if (info.valid) {
                    return info;
                }
            }
        }
    }

    // 4. 扫描系统级和用户级的 JavaVirtualMachines 目录
    std::vector<JavaInfo> candidates;
    ScanJavaVirtualMachines("/Library/Java", candidates);
    const char* home = getenv("HOME");
    if (home) {
        ScanJavaVirtualMachines(std::string(home) + "/Library/Java", candidates);
    }

    // 选择版本最高的 JDK
    if (!candidates.empty()) {
        std::sort(candidates.begin(), candidates.end(),
            [](const JavaInfo& a, const JavaInfo& b) {
                auto parse = [](const std::string& v) -> std::vector<int> {
                    std::vector<int> parts;
                    std::string cur;
                    for (char c : v) {
                        if (c == '.') {
                            parts.push_back(std::atoi(cur.c_str()));
                            cur.clear();
                        } else if (c >= '0' && c <= '9') {
                            cur += c;
                        } else { break; }
                    }
                    if (!cur.empty()) parts.push_back(std::atoi(cur.c_str()));
                    return parts;
                };
                auto va = parse(a.version);
                auto vb = parse(b.version);
                for (size_t i = 0; i < va.size() && i < vb.size(); i++)
                    if (va[i] != vb[i]) return va[i] > vb[i];
                return va.size() > vb.size();
            });
        return candidates[0];
    }

    // 5. 最后尝试跟踪 /usr/bin/java 的符号链接找到实际 JDK
    fs::path usrBinJava("/usr/bin/java");
    if (fs::exists(usrBinJava)) {
        LOG_VERBOSE("Checking /usr/bin/java", true);
        std::string resolved;
        if (RunCommandCapture("readlink -f /usr/bin/java 2>/dev/null", resolved)) {
            resolved.erase(resolved.find_last_not_of(" \n\r\t") + 1);
            if (!resolved.empty()) {
                fs::path resolvedPath(resolved);
                fs::path candidate = resolvedPath.parent_path().parent_path();
                JavaInfo info = CheckJavaPath(candidate);
                if (info.valid) {
                    return info;
                }
            }
        }
    }

    return JavaInfo();
}

// 递归复制目录内容（支持普通文件、目录、符号链接）
static bool CopyDir(const fs::path& src, const fs::path& dst, bool verbose) {
    try {
        fs::create_directories(dst);
        for (auto& entry : fs::directory_iterator(src)) {
            fs::path destPath = dst / entry.path().filename();
            if (fs::exists(destPath)) {
                fs::remove_all(destPath);
            }
            if (entry.is_symlink()) {
                fs::path target = fs::read_symlink(entry.path());
                if (target.is_relative()) {
                    target = fs::absolute(entry.path().parent_path() / target);
                }
                fs::create_symlink(target, destPath);
                LOG_VERBOSE("Symlink " << fs::relative(destPath, dst), verbose);
            } else if (entry.is_regular_file() || entry.is_socket() || entry.is_fifo()) {
                fs::copy_file(entry.path(), destPath, fs::copy_options::overwrite_existing);
                LOG_VERBOSE("Copied " << fs::relative(destPath, dst), verbose);
            } else if (entry.is_directory()) {
                CopyDir(entry.path(), destPath, verbose);
            }
        }
    } catch (const fs::filesystem_error& e) {
        LOG_ERROR("Failed to copy: {}", e.what());
        return false;
    }
    return true;
}

// 将 Java 运行时复制到 .app 内，跳过运行时不必要的内容（man、include、jmods、demo、sample）
bool BundleJava(const fs::path& javaHome, const fs::path& destDir, bool verbose) {
    LOG_VERBOSE("Bundling Java from " << javaHome << " to " << destDir, verbose);

    // 运行时不需要的目录列表
    std::set<std::string> skipNames = {
        "man",
        "include",
        "jmods",
        "demo",
        "sample"
    };

    try {
        fs::create_directories(destDir);
        for (auto& entry : fs::directory_iterator(javaHome)) {
            std::string name = entry.path().filename().string();
            // 跳过非运行所需的目录
            if (entry.is_directory() && skipNames.count(name)) {
                LOG_VERBOSE("Skipping " << name, verbose);
                continue;
            }
            // 跳过 Java 源码包
            if (name == "src.zip" || name == "src.jar") {
                LOG_VERBOSE("Skipping " << name, verbose);
                continue;
            }
            fs::path destPath = destDir / name;
            if (fs::exists(destPath)) {
                fs::remove_all(destPath);
            }
            if (entry.is_directory()) {
                if (!CopyDir(entry.path(), destPath, verbose)) {
                    return false;
                }
            } else if (entry.is_symlink()) {
                fs::path target = fs::read_symlink(entry.path());
                fs::create_symlink(target, destPath);
            } else {
                fs::copy_file(entry.path(), destPath, fs::copy_options::overwrite_existing);
            }
        }

        // 额外清理 lib/src.zip
        fs::path libSrcZip = destDir / "lib" / "src.zip";
        if (fs::exists(libSrcZip)) {
            fs::remove(libSrcZip);
            LOG_VERBOSE("Removed lib/src.zip", verbose);
        }
    } catch (const fs::filesystem_error& e) {
        LOG_ERROR("Failed to bundle Java: {}", e.what());
        return false;
    }
    return true;
}
