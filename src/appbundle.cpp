#include "appbundle.h"
#include "javabundle.h"
#include "utils.h"
#include <fstream>
#include <sstream>
#include <iostream>

static bool WriteInfoPlist(const fs::path& plistPath, const std::string& appName, const std::string& version) {
    std::ostringstream plist;
    plist << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    plist << "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" "
             "\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n";
    plist << "<plist version=\"1.0\">\n";
    plist << "<dict>\n";
    plist << "    <key>CFBundleName</key>\n";
    plist << "    <string>" << appName << "</string>\n";
    plist << "    <key>CFBundleDisplayName</key>\n";
    plist << "    <string>" << appName << "</string>\n";
    plist << "    <key>CFBundleIdentifier</key>\n";
    plist << "    <string>com.hmcl." << appName << "</string>\n";
    plist << "    <key>CFBundleVersion</key>\n";
    plist << "    <string>" << version << "</string>\n";
    plist << "    <key>CFBundleShortVersionString</key>\n";
    plist << "    <string>" << version << "</string>\n";
    plist << "    <key>CFBundleIconFile</key>\n";
    plist << "    <string>" << appName << "</string>\n";
    plist << "    <key>CFBundleExecutable</key>\n";
    plist << "    <string>launch.sh</string>\n";
    plist << "    <key>CFBundlePackageType</key>\n";
    plist << "    <string>APPL</string>\n";
    plist << "    <key>LSMinimumSystemVersion</key>\n";
    plist << "    <string>10.15</string>\n";
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
                     const std::string& version, bool verbose,
                     const fs::path& javaHome) {
    fs::path appDir = config.outputDir / (config.appName + ".app");
    fs::path contentsDir = appDir / "Contents";
    fs::path macosDir = contentsDir / "MacOS";
    fs::path resourcesDir = contentsDir / "Resources";

    LOG_VERBOSE("Creating .app bundle at " << appDir, verbose);

    fs::create_directories(macosDir);
    fs::create_directories(resourcesDir);

    fs::path plistPath = contentsDir / "Info.plist";
    if (!WriteInfoPlist(plistPath, config.appName, version)) {
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
