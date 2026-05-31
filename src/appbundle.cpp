#include "appbundle.h"
#include "javabundle.h"
#include "utils.h"
#include <fstream>
#include <sstream>
#include <iostream>

// 从构建元信息中组装系统版本字符串（如 "macOS 14.5 (23F79) arm64"）
static std::string BuildSystemString(const BuildInfo& info) {
    std::string s;
    if (!info.macOSVersion.empty()) {
        s += "macOS " + info.macOSVersion;
        if (!info.macOSBuild.empty()) {
            s += " (" + info.macOSBuild + ")";
        }
        if (!info.architecture.empty()) {
            s += " " + info.architecture;
        }
    }
    return s;
}

// 向 plist 写入一个 <key>/<string> 键值对
static void WritePlistString(std::ostringstream& plist, const std::string& key, const std::string& value) {
    plist << "    <key>" << key << "</key>\n";
    plist << "    <string>" << value << "</string>\n";
}

// 生成 Info.plist：包含标准 macOS 字段和自定义的 HMCLMacBuilder 元信息字段
static bool WriteInfoPlist(const fs::path& plistPath, const std::string& appName, const std::string& version, const BuildInfo& buildInfo) {
    std::ostringstream plist;
    plist << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    plist << "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" "
             "\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n";
    plist << "<plist version=\"1.0\">\n";
    plist << "<dict>\n";

    // macOS 标准 plist 字段
    WritePlistString(plist, "CFBundleName", appName);
    WritePlistString(plist, "CFBundleDisplayName", appName);
    WritePlistString(plist, "CFBundleIdentifier", "com.hmcl." + appName);
    WritePlistString(plist, "CFBundleVersion", version);
    WritePlistString(plist, "CFBundleShortVersionString", version);
    WritePlistString(plist, "CFBundleIconFile", appName);
    WritePlistString(plist, "CFBundleExecutable", "launch.sh");
    WritePlistString(plist, "CFBundlePackageType", "APPL");
    WritePlistString(plist, "LSMinimumSystemVersion", "10.15");

    // 自定义构建元信息字段（可在 Finder 或系统信息中查看）
    WritePlistString(plist, "HMCLMacBuilderName", buildInfo.builderName);
    WritePlistString(plist, "HMCLMacBuilderVersion", buildInfo.builderVersion);
    WritePlistString(plist, "HMCLMacBuilderRepository", buildInfo.builderRepo);
    WritePlistString(plist, "HMCLMacBuilderDate", buildInfo.buildDate + " " + buildInfo.buildTime);
    WritePlistString(plist, "HMCLMacBuilderArguments", buildInfo.buildArgs);

    std::string sysStr = BuildSystemString(buildInfo);
    if (!sysStr.empty()) {
        WritePlistString(plist, "HMCLMacBuilderSystem", sysStr);
    }
    if (!buildInfo.compilerVersion.empty()) {
        WritePlistString(plist, "HMCLMacBuilderCompiler", buildInfo.compilerVersion);
    }
    if (!buildInfo.cmakeVersion.empty()) {
        WritePlistString(plist, "HMCLMacBuilderCMake", buildInfo.cmakeVersion);
    }
    if (!buildInfo.xcodeVersion.empty()) {
        WritePlistString(plist, "HMCLMacBuilderXcode", buildInfo.xcodeVersion);
    }
    if (!buildInfo.javaVersion.empty()) {
        WritePlistString(plist, "HMCLMacBuilderJavaVersion", buildInfo.javaVersion);
        WritePlistString(plist, "HMCLMacBuilderJavaHome", buildInfo.javaHome);
    }
    if (!buildInfo.createDmgVersion.empty()) {
        WritePlistString(plist, "HMCLMacBuilderCreateDmg", buildInfo.createDmgVersion);
    }

    plist << "</dict>\n";
    plist << "</plist>\n";

    if (!WriteFile(plistPath, plist.str())) {
        LOG_ERROR("Failed to write Info.plist to {}", plistPath);
        return false;
    }
    return true;
}

// 创建完整的 .app 包：构建目录结构 → 写入 Info.plist → 复制 JAR/ICNS/启动脚本 → 可选打包 Java
bool CreateAppBundle(const Config& config, const fs::path& jarPath,
                     const fs::path& icnsPath, const fs::path& launcherPath,
                     const std::string& version, const BuildInfo& buildInfo,
                     bool verbose, const fs::path& javaHome,
                     const LicenseInfo& licenseInfo) {
    fs::path appDir = config.outputDir / (config.appName + ".app");
    fs::path contentsDir = appDir / "Contents";
    fs::path macosDir = contentsDir / "MacOS";
    fs::path resourcesDir = contentsDir / "Resources";

    LOG_VERBOSE("Creating .app bundle at " << appDir, verbose);

    // 创建标准 .app 目录结构
    fs::create_directories(macosDir);
    fs::create_directories(resourcesDir);

    // 生成并写入 Info.plist
    fs::path plistPath = contentsDir / "Info.plist";
    if (!WriteInfoPlist(plistPath, config.appName, version, buildInfo)) {
        return false;
    }

    // 复制 HMCL JAR 到 Resources
    fs::path jarDest = resourcesDir / (config.appName + ".jar");
    try {
        fs::copy_file(jarPath, jarDest, fs::copy_options::overwrite_existing);
        LOG_VERBOSE("Copied JAR to " << jarDest, verbose);
    } catch (const fs::filesystem_error& e) {
        LOG_ERROR("Failed to copy JAR: {}", e.what());
        return false;
    }

    // 复制 ICNS 图标到 Resources（可选）
    if (fs::exists(icnsPath)) {
        fs::path icnsDest = resourcesDir / (config.appName + ".icns");
        try {
            fs::copy_file(icnsPath, icnsDest, fs::copy_options::overwrite_existing);
            LOG_VERBOSE("Copied ICNS to " << icnsDest, verbose);
        } catch (const fs::filesystem_error& e) {
            LOG_ERROR("Failed to copy ICNS: {}", e.what());
            return false;
        }
    } else {
        LOG_VERBOSE("ICNS not found, skipping icon", verbose);
    }

    // 复制启动脚本到 MacOS 目录并设置为可执行
    fs::path launcherDest = macosDir / "launch.sh";
    try {
        fs::copy_file(launcherPath, launcherDest, fs::copy_options::overwrite_existing);
        fs::permissions(launcherDest,
            fs::perms::owner_all | fs::perms::group_read | fs::perms::group_exec |
            fs::perms::others_read | fs::perms::others_exec);
        LOG_VERBOSE("Copied launcher to " << launcherDest, verbose);
    } catch (const fs::filesystem_error& e) {
        LOG_ERROR("Failed to copy launcher: {}", e.what());
        return false;
    }

    // 复制开源协议文件到 Resources/licenses/
    if (licenseInfo.valid() && fs::exists(licenseInfo.licensesDir)) {
        fs::path licensesDest = resourcesDir / "licenses";
        fs::create_directories(licensesDest);
        try {
            for (auto& entry : fs::directory_iterator(licenseInfo.licensesDir)) {
                fs::path dest = licensesDest / entry.path().filename();
                fs::copy_file(entry.path(), dest, fs::copy_options::overwrite_existing);
                LOG_VERBOSE("Copied license " << entry.path().filename() << " to " << dest, verbose);
            }
        } catch (const fs::filesystem_error& e) {
            LOG_WARNING("Failed to copy license files: {}", e.what());
        }
    } else {
        LOG_VERBOSE("No license files to copy (not valid or dir missing)", verbose);
    }

    // 可选：打包 Java 运行时的 Contents/Java 目录
    if (!javaHome.empty()) {
        fs::path javaDir = contentsDir / "Java";
        LOG_INFO("Bundling Java runtime into .app...");
        if (!BundleJava(javaHome, javaDir, verbose)) {
            LOG_ERROR("Failed to bundle Java runtime");
            return false;
        }
        LOG_INFO("Java runtime bundled successfully");
    }

    LOG_INFO("App bundle created at {}", appDir);
    return true;
}
