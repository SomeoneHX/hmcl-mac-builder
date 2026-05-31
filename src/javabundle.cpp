#include "javabundle.h"
#include "utils.h"
#include "i18n.h"
#include <iostream>
#include <regex>
#include <algorithm>
#include <cstdlib>
#include <vector>
#include <set>

static bool GetJavaVersion(const fs::path& javaBinary, std::string& version) {
    std::string cmd = "\"" + javaBinary.string() + "\" -version 2>&1";
    std::string output;
    if (!RunCommandCapture(cmd, output)) {
        return false;
    }
    std::regex versionRegex("\"(\\d+\\.\\d+\\.\\d+[_+][^\\s\"]*|\\d+\\.\\d+\\.\\d+|\\d+)");
    std::smatch match;
    if (std::regex_search(output, match, versionRegex)) {
        version = match[1];
        return true;
    }
    versionRegex = std::regex("(\\d+)\\.(\\d+)\\.(\\d+)");
    if (std::regex_search(output, match, versionRegex)) {
        version = match[0];
        return true;
    }
    return false;
}

static int ParseMajorVersion(const std::string& version) {
    size_t dot = version.find('.');
    if (dot == std::string::npos) {
        return std::atoi(version.c_str());
    }
    return std::atoi(version.substr(0, dot).c_str());
}

static bool IsValidJavaHome(const fs::path& javaHome, std::string& version) {
    fs::path javaBin = javaHome / "bin" / "java";
    if (!fs::exists(javaBin)) {
        return false;
    }
    return GetJavaVersion(javaBin, version);
}

static bool IsJavaVersionSatisfies(const std::string& version, int minMajor) {
    int major = ParseMajorVersion(version);
    return major >= minMajor;
}

static JavaInfo CheckJavaPath(const fs::path& path) {
    JavaInfo info;
    fs::path javaHome;
    fs::path candidate = path;
    if (fs::exists(candidate / "bin" / "java")) {
        javaHome = candidate;
    } else if (fs::exists(candidate / "Contents" / "Home" / "bin" / "java")) {
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

JavaInfo FindJava(const std::string& userPath) {
    if (!userPath.empty()) {
        LOG_VERBOSE("Checking user-specified Java path: " << userPath, true);
        JavaInfo info = CheckJavaPath(userPath);
        if (info.valid) {
            return info;
        }
        LOG_WARNING("User-specified Java path is invalid: {}", userPath);
    }

    const char* javaHomeEnv = std::getenv("JAVA_HOME");
    if (javaHomeEnv && javaHomeEnv[0] != '\0') {
        LOG_VERBOSE("Checking JAVA_HOME: " << javaHomeEnv, true);
        JavaInfo info = CheckJavaPath(javaHomeEnv);
        if (info.valid) {
            return info;
        }
    }

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

    std::vector<JavaInfo> candidates;
    ScanJavaVirtualMachines("/Library/Java", candidates);
    ScanJavaVirtualMachines(std::string(getenv("HOME")) + "/Library/Java", candidates);

    if (!candidates.empty()) {
        std::sort(candidates.begin(), candidates.end(),
            [](const JavaInfo& a, const JavaInfo& b) {
                return ParseMajorVersion(a.version) > ParseMajorVersion(b.version);
            });
        return candidates[0];
    }

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

bool BundleJava(const fs::path& javaHome, const fs::path& destDir, bool verbose) {
    LOG_VERBOSE("Bundling Java from " << javaHome << " to " << destDir, verbose);

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
            if (entry.is_directory() && skipNames.count(name)) {
                LOG_VERBOSE("Skipping " << name, verbose);
                continue;
            }
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
