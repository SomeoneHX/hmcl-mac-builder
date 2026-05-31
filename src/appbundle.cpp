#include "appbundle.h"
#include "javabundle.h"
#include "utils.h"
#include <fstream>
#include <sstream>
#include <iostream>

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

static void WritePlistString(std::ostringstream& plist, const std::string& key, const std::string& value) {
    plist << "    <key>" << key << "</key>\n";
    plist << "    <string>" << value << "</string>\n";
}

static bool WriteInfoPlist(const fs::path& plistPath, const std::string& appName, const std::string& version, const BuildInfo& buildInfo) {
    std::ostringstream plist;
    plist << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    plist << "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" "
             "\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n";
    plist << "<plist version=\"1.0\">\n";
    plist << "<dict>\n";

    WritePlistString(plist, "CFBundleName", appName);
    WritePlistString(plist, "CFBundleDisplayName", appName);
    WritePlistString(plist, "CFBundleIdentifier", "com.hmcl." + appName);
    WritePlistString(plist, "CFBundleVersion", version);
    WritePlistString(plist, "CFBundleShortVersionString", version);
    WritePlistString(plist, "CFBundleIconFile", appName);
    WritePlistString(plist, "CFBundleExecutable", "launch.sh");
    WritePlistString(plist, "CFBundlePackageType", "APPL");
    WritePlistString(plist, "LSMinimumSystemVersion", "10.15");

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

bool CreateAppBundle(const Config& config, const fs::path& jarPath,
                     const fs::path& icnsPath, const fs::path& launcherPath,
                     const std::string& version, const BuildInfo& buildInfo,
                     bool verbose, const fs::path& javaHome) {
    fs::path appDir = config.outputDir / (config.appName + ".app");
    fs::path contentsDir = appDir / "Contents";
    fs::path macosDir = contentsDir / "MacOS";
    fs::path resourcesDir = contentsDir / "Resources";

    LOG_VERBOSE("Creating .app bundle at " << appDir, verbose);

    fs::create_directories(macosDir);
    fs::create_directories(resourcesDir);

    fs::path plistPath = contentsDir / "Info.plist";
    if (!WriteInfoPlist(plistPath, config.appName, version, buildInfo)) {
        return false;
    }

    fs::path jarDest = resourcesDir / (config.appName + ".jar");
    try {
        fs::copy_file(jarPath, jarDest, fs::copy_options::overwrite_existing);
        LOG_VERBOSE("Copied JAR to " << jarDest, verbose);
    } catch (const fs::filesystem_error& e) {
        LOG_ERROR("Failed to copy JAR: {}", e.what());
        return false;
    }

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
