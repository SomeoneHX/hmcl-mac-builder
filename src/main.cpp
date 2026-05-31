#include "config.h"
#include "version.h"
#include "utils.h"
#include "network.h"
#include "icon.h"
#include "launcher.h"
#include "appbundle.h"
#include "javabundle.h"
#include "dmg.h"
#include <iostream>
#include <future>
#include <cstdlib>
#include <ctime>
#include <sstream>

static int savedArgc = 0;
static char** savedArgv = nullptr;

static std::string ReconstructArgs() {
    std::ostringstream ss;
    for (int i = 1; i < savedArgc; i++) {
        std::string arg(savedArgv[i]);
        if (arg.find(' ') != std::string::npos) {
            ss << " \"" << arg << "\"";
        } else {
            ss << " " << arg;
        }
    }
    return ss.str();
}

static BuildInfo CollectBuildInfo(const Config& config, const JavaInfo& javaInfo) {
    BuildInfo info;

    info.builderVersion = HMCL_MAC_BUILDER_VERSION;
    info.builderName = HMCL_MAC_BUILDER_NAME;
    info.builderRepo = HMCL_MAC_BUILDER_REPO;

    std::time_t now = std::time(nullptr);
    char dateBuf[11];
    std::strftime(dateBuf, sizeof(dateBuf), "%Y-%m-%d", std::localtime(&now));
    info.buildDate = dateBuf;
    char timeBuf[9];
    std::strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", std::localtime(&now));
    info.buildTime = timeBuf;

    info.buildArgs = ReconstructArgs();

    info.macOSVersion = CaptureOutput("sw_vers -productVersion 2>/dev/null");
    info.macOSBuild = CaptureOutput("sw_vers -buildVersion 2>/dev/null");
    info.architecture = CaptureOutput("uname -m 2>/dev/null");
    info.compilerVersion = CaptureOutput("c++ --version 2>/dev/null | head -1");
    info.cmakeVersion = CaptureOutput("cmake --version 2>/dev/null | head -1");

    std::string xcode = CaptureOutput("xcodebuild -version 2>/dev/null | head -1");
    if (!xcode.empty()) {
        std::string xcodeBuild = CaptureOutput("xcodebuild -version 2>/dev/null | tail -1");
        info.xcodeVersion = xcode + " (" + xcodeBuild + ")";
    }

    if (javaInfo.valid) {
        info.javaVersion = javaInfo.version;
        info.javaHome = javaInfo.javaHome.string();
    }

    info.createDmgVersion = CaptureOutput("create-dmg --version 2>/dev/null");

    return info;
}

static bool CleanOutput(const Config& config) {
    fs::path appPath = config.outputDir / (config.appName + ".app");
    bool cleaned = false;
    if (fs::exists(appPath)) {
        fs::remove_all(appPath);
        LOG_INFO("Removed {}", appPath);
        cleaned = true;
    }
    for (auto& entry : fs::directory_iterator(config.outputDir)) {
        if (entry.path().extension() == ".dmg") {
            fs::remove(entry.path());
            LOG_INFO("Removed {}", entry.path());
            cleaned = true;
        }
    }
    if (!cleaned) {
        LOG_INFO("Nothing to clean in {}", config.outputDir);
    }
    return true;
}

int main(int argc, char* argv[]) {
    savedArgc = argc;
    savedArgv = argv;

    Config config;
    if (!ParseArgs(argc, argv, config)) {
        PrintUsage(argv[0]);
        return EXIT_FAILURE;
    }

    if (config.lang.empty()) {
        config.lang = I18n::detectLang();
    }
    I18n::instance().setLang(config.lang);

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
        LOG_INFO("Found version: {}", version);
    }

    fs::path icnsPath = tempDir.path() / (config.appName + ".icns");
    fs::path launcherPath = tempDir.path() / "launch.sh";
    fs::path jarDestPath = tempDir.path() / (config.appName + ".jar");

    std::future<bool> iconFuture;
    std::future<bool> jarFuture;

    if (!config.skipIcon) {
        iconFuture = std::async(std::launch::async, [&]() {
            return ProcessIcon(icnsPath, config.verbose, config.proxyUrl);
        });
    }

    if (config.jarPath.empty()) {
        LOG_INFO("Downloading HMCL {}...", version);
        jarFuture = std::async(std::launch::async, [&]() {
            return DownloadFile(release.jarUrl, jarDestPath, config.proxyUrl);
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

    std::string javaHome;
    JavaInfo javaInfo;
    if (config.bundleJava) {
        LOG_INFO("Detecting local Java...");
        javaInfo = FindJava(config.javaPath);
        if (!javaInfo.valid) {
            LOG_ERROR("Failed to find Java 17+. Please install JDK 17 or later, or use --java-path to specify a path.");
            return EXIT_FAILURE;
        }
        LOG_INFO("Found Java {} at {}", javaInfo.version, javaInfo.javaHome);
        javaHome = javaInfo.javaHome.string();
    }

    BuildInfo buildInfo = CollectBuildInfo(config, javaInfo);

    LOG_INFO("Generating launcher script...");
    if (!GenerateLauncherScript(launcherPath, config.appName, version, buildInfo, config.bundleJava)) {
        LOG_ERROR("Failed to generate launcher script");
        return EXIT_FAILURE;
    }

    LOG_INFO("Creating app bundle...");
    if (!CreateAppBundle(config, jarPath, icnsPath, launcherPath, version, buildInfo, config.verbose, javaHome)) {
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
        LOG_INFO("Temporary files kept at {}", tempDir.path());
    }

    LOG_INFO("Done!");
    return EXIT_SUCCESS;
}
