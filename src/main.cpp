// main.cpp — 程序入口，编排整个构建流程：
//   解析参数 → 获取 HMCL JAR → 生成图标 → 检测 Java → 下载协议 →
//   生成启动脚本 → 打包 .app → 创建 DMG → 清理临时文件
#include "config.h"
#include "version.h"
#include "utils.h"
#include "network.h"
#include "icon.h"
#include "launcher.h"
#include "appbundle.h"
#include "javabundle.h"
#include "dmg.h"
#include "license.h"
#include <iostream>
#include <future>
#include <cstdlib>
#include <ctime>
#include <sstream>

static int savedArgc = 0;
static char** savedArgv = nullptr;

// 重建原始命令行参数字符串（含空格引号处理，嵌入启动脚本元信息）
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

// 采集构建环境信息：工具版本、系统版本、编译器、架构等，用于嵌入启动脚本和 Info.plist
static BuildInfo CollectBuildInfo(const Config& config, const JavaInfo& javaInfo) {
    BuildInfo info;

    info.builderVersion = HMCL_MAC_BUILDER_VERSION;
    info.builderName = HMCL_MAC_BUILDER_NAME;
    info.builderRepo = HMCL_MAC_BUILDER_REPO;
    info.builderLang = config.lang;

    std::time_t now = std::time(nullptr);
    char dateBuf[11];
    std::strftime(dateBuf, sizeof(dateBuf), "%Y-%m-%d", std::localtime(&now));
    info.buildDate = dateBuf;
    char timeBuf[9];
    std::strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", std::localtime(&now));
    info.buildTime = timeBuf;

    info.buildArgs = ReconstructArgs();

    // 环境探测：系统版本、架构、编译器、构建工具版本
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
        info.javaArchitecture = javaInfo.architecture;
        info.javaHome = javaInfo.javaHome.string();
    }

    info.createDmgVersion = CaptureOutput("create-dmg --version 2>/dev/null");

    return info;
}

// 清理输出目录：删除 .app 文件夹和所有 .dmg 文件
static bool CleanOutput(const Config& config) {
    fs::path appPath = config.outputDir / (config.appName + ".app");
    bool cleaned = false;
    try {
        if (fs::exists(appPath)) {
            fs::remove_all(appPath);
            LOG_INFO("Removed {}", appPath);
            cleaned = true;
        }
        // 先收集 .dmg 路径，再删除，避免遍历时删除当前条目的 UB
        std::vector<fs::path> dmgFiles;
        if (fs::exists(config.outputDir)) {
            for (auto& entry : fs::directory_iterator(config.outputDir)) {
                if (entry.path().extension() == ".dmg") {
                    dmgFiles.push_back(entry.path());
                }
            }
        }
        for (const auto& dmgPath : dmgFiles) {
            fs::remove(dmgPath);
            LOG_INFO("Removed {}", dmgPath);
            cleaned = true;
        }
    } catch (const fs::filesystem_error& e) {
        LOG_WARNING("Clean failed for part of the output: {}", e.what());
    }
    if (!cleaned) {
        LOG_INFO("Nothing to clean in {}", config.outputDir);
    }
    return true;
}

int main(int argc, char* argv[]) {
    savedArgc = argc;
    savedArgv = argv;

    // 阶段 1：解析命令行参数
    Config config;
    if (!ParseArgs(argc, argv, config)) {
        PrintUsage(argv[0]);
        return EXIT_FAILURE;
    }

    // 语言检测与国际化初始化
    if (config.lang.empty()) {
        config.lang = I18n::detectLang();
    }
    I18n::instance().setLang(config.lang);

    // 创建输出目录
    fs::create_directories(config.outputDir);

    // 如果仅需清理，执行清理后立即退出
    if (config.clean) {
        CleanOutput(config);
        return EXIT_SUCCESS;
    }

    // 阶段 2：创建临时工作目录
    TempDir tempDir;
    LOG_VERBOSE("Using temporary directory: " << tempDir.path(), config.verbose);

    // 阶段 3：获取 HMCL JAR（本地或从 GitHub 下载）
    ReleaseInfo release;
    std::string version;
    std::string jarPath;

    if (!config.jarPath.empty()) {
        // 使用用户提供的本地 JAR
        jarPath = config.jarPath;
        if (!config.tag.empty()) {
            version = config.tag;
        } else {
            std::string extracted = ExtractVersionFromJarName(config.jarPath);
            if (!extracted.empty()) {
                version = extracted;
                LOG_INFO("Auto-detected HMCL version {} from filename", version);
            } else {
                version = "local";
            }
        }
        LOG_VERBOSE("Using local JAR: " << jarPath, config.verbose);
        LOG_VERBOSE("Version set to: " << version, config.verbose);
    } else {
        // 从 GitHub API 获取最新 Release 信息
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

    // 定义临时目录中的中间文件路径
    fs::path icnsPath = tempDir.path() / (config.appName + ".icns");
    fs::path launcherPath = tempDir.path() / "launch.sh";
    fs::path jarDestPath = tempDir.path() / (config.appName + ".jar");

    // 阶段 4：并行执行耗时任务——图标处理和 JAR 下载
    std::future<bool> iconFuture;
    std::future<bool> jarFuture;

    // 异步下载并转换图标（跳过图标时不做）
    if (!config.skipIcon) {
        iconFuture = std::async(std::launch::async, [&]() {
            return ProcessIcon(icnsPath, config.verbose, config.proxyUrl);
        });
    }

    // 异步下载 JAR（使用本地 JAR 时不需要）
    if (config.jarPath.empty()) {
        LOG_INFO("Downloading HMCL {}...", version);
        jarFuture = std::async(std::launch::async, [&]() {
            return DownloadFile(release.jarUrl, jarDestPath, config.proxyUrl);
        });
        jarPath = jarDestPath.string();
    }

    // 等待图标处理完成（非关键，失败则警告后继续）
    if (!config.skipIcon) {
        bool iconOk = iconFuture.get();
        if (!iconOk) {
            LOG_WARNING("Icon processing failed, proceeding without custom icon.");
        }
    }

    // 等待 JAR 下载完成（关键，失败则退出）
    if (!config.jarPath.empty()) {
        // 使用本地 JAR，无需等待
    } else {
        bool jarOk = jarFuture.get();
        if (!jarOk) {
            LOG_ERROR("Failed to download HMCL JAR");
            return EXIT_FAILURE;
        }
        LOG_INFO("JAR downloaded successfully");
    }

    // 阶段 5：检测 Java 运行时（仅当需要打包 Java 时）
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

    // 阶段 6：下载开源协议文件（可选跳过）
    LicenseInfo licenseInfo;
    if (!config.skipLicenses) {
        LOG_INFO("Downloading open-source licenses...");
        licenseInfo = DownloadLicenses(tempDir.path(), config.bundleJava, javaHome, config.proxyUrl);
    } else {
        LOG_VERBOSE("Skipping license download", config.verbose);
    }

    // 阶段 7：采集构建元信息
    BuildInfo buildInfo = CollectBuildInfo(config, javaInfo);

    // 阶段 8：生成启动脚本
    LOG_INFO("Generating launcher script...");
    if (!GenerateLauncherScript(launcherPath, config.appName, version, buildInfo, config.bundleJava)) {
        LOG_ERROR("Failed to generate launcher script");
        return EXIT_FAILURE;
    }

    // 阶段 9：组装 .app 包
    LOG_INFO("Creating app bundle...");
    if (!CreateAppBundle(config, jarPath, icnsPath, launcherPath, version, buildInfo, config.verbose, javaHome, licenseInfo)) {
        LOG_ERROR("Failed to create app bundle");
        return EXIT_FAILURE;
    }

    // 阶段 10：创建 DMG（可选）
    if (!config.noDmg) {
        LOG_INFO("Creating DMG...");
        CreateDMG(config, version, config.verbose, buildInfo.javaVersion, buildInfo.javaArchitecture);
    }

    // 阶段 11：清理临时文件
    if (!config.keepTemp) {
        LOG_VERBOSE("Cleaning up temporary directory", config.verbose);
    } else {
        tempDir.release();
        LOG_INFO("Temporary files kept at {}", tempDir.path());
    }

    LOG_INFO("Done!");
    return EXIT_SUCCESS;
}
