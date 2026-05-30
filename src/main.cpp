#include "config.h"
#include "utils.h"
#include "network.h"
#include "icon.h"
#include "launcher.h"
#include "appbundle.h"
#include "dmg.h"
#include <iostream>
#include <future>
#include <cstdlib>

static bool CleanOutput(const Config& config) {
    fs::path appPath = config.outputDir / (config.appName + ".app");
    bool cleaned = false;
    if (fs::exists(appPath)) {
        fs::remove_all(appPath);
        LOG_INFO("Removed " << appPath);
        cleaned = true;
    }
    for (auto& entry : fs::directory_iterator(config.outputDir)) {
        if (entry.path().extension() == ".dmg") {
            fs::remove(entry.path());
            LOG_INFO("Removed " << entry.path());
            cleaned = true;
        }
    }
    if (!cleaned) {
        LOG_INFO("Nothing to clean in " << config.outputDir);
    }
    return true;
}

int main(int argc, char* argv[]) {
    Config config;
    if (!ParseArgs(argc, argv, config)) {
        PrintUsage(argv[0]);
        return EXIT_FAILURE;
    }

    fs::create_directories(config.outputDir);

    if (config.clean) {
        CleanOutput(config);
        return EXIT_SUCCESS;
    }

    TempDir tempDir;
    LOG_VERBOSE("Using temporary directory: " << tempDir.path(), config.verbose);

    ReleaseInfo release;
    std::string version;
    std::string jarPath;

    if (!config.jarPath.empty()) {
        jarPath = config.jarPath;
        version = config.tag.empty() ? "local" : config.tag;
        LOG_VERBOSE("Using local JAR: " << jarPath, config.verbose);
        LOG_VERBOSE("Version set to: " << version, config.verbose);
    } else {
        LOG_INFO("Fetching latest release info from GitHub...");
        release = GetLatestRelease(config.tag);
        if (!release.valid) {
            LOG_ERROR("Failed to get release info. Use --jar to specify a local file.");
            return EXIT_FAILURE;
        }
        if (release.jarUrl.empty()) {
            LOG_ERROR("No JAR file found in the release.");
            return EXIT_FAILURE;
        }
        version = release.version;
        jarPath = tempDir.path() / (config.appName + ".jar");
        LOG_INFO("Found version: " << version);
    }

    fs::path icnsPath = tempDir.path() / (config.appName + ".icns");
    fs::path launcherPath = tempDir.path() / "launch.sh";
    fs::path jarDestPath = tempDir.path() / (config.appName + ".jar");

    std::future<bool> iconFuture;
    std::future<bool> jarFuture;

    if (!config.skipIcon) {
        iconFuture = std::async(std::launch::async, [&]() {
            return ProcessIcon(icnsPath, config.verbose);
        });
    }

    if (config.jarPath.empty()) {
        LOG_INFO("Downloading HMCL " << version << "...");
        jarFuture = std::async(std::launch::async, [&]() {
            return DownloadFile(release.jarUrl, jarDestPath);
        });
        jarPath = jarDestPath.string();
    }

    if (!config.skipIcon) {
        bool iconOk = iconFuture.get();
        if (!iconOk) {
            LOG_WARNING("Icon processing failed, proceeding without custom icon.");
        }
    }

    if (!config.jarPath.empty()) {
        // using local jar, nothing to wait for
    } else {
        bool jarOk = jarFuture.get();
        if (!jarOk) {
            LOG_ERROR("Failed to download HMCL JAR");
            return EXIT_FAILURE;
        }
        LOG_INFO("JAR downloaded successfully");
    }

    LOG_INFO("Generating launcher script...");
    if (!GenerateLauncherScript(launcherPath, config.appName, version)) {
        LOG_ERROR("Failed to generate launcher script");
        return EXIT_FAILURE;
    }

    LOG_INFO("Creating app bundle...");
    if (!CreateAppBundle(config, jarPath, icnsPath, launcherPath, version, config.verbose)) {
        LOG_ERROR("Failed to create app bundle");
        return EXIT_FAILURE;
    }

    if (!config.noDmg) {
        LOG_INFO("Creating DMG...");
        CreateDMG(config, version, config.verbose);
    }

    if (!config.keepTemp) {
        LOG_VERBOSE("Cleaning up temporary directory", config.verbose);
    } else {
        tempDir.release();
        LOG_INFO("Temporary files kept at " << tempDir.path());
    }

    LOG_INFO("Done!");
    return EXIT_SUCCESS;
}
