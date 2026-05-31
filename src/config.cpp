// config.cpp — 命令行参数解析实现
#include "config.h"
#include "version.h"
#include "utils.h"
#include "i18n.h"
#include <iostream>
#include <cstring>
#include <cstdlib>

// 判断 argv[i] 是否匹配短参数或长参数
static bool IsFlag(int argc, char* argv[], int i, const std::string& shortFlag, const std::string& longFlag) {
    return strcmp(argv[i], shortFlag.c_str()) == 0 || strcmp(argv[i], longFlag.c_str()) == 0;
}

// 判断 argv[i] 是否带有后续参数值
static bool HasArg(int argc, char* argv[], int i) {
    return i + 1 < argc;
}

// 解析命令行参数，逐项填充 Config 结构体
bool ParseArgs(int argc, char* argv[], Config& config) {
    config.lang = I18n::detectLang();
    I18n::instance().setLang(config.lang);

    for (int i = 1; i < argc; i++) {
        if (IsFlag(argc, argv, i, "", "--lang")) {
            // 切换输出语言
            if (!HasArg(argc, argv, i)) {
                LOG_ERROR("--lang requires an argument (en or zh)");
                return false;
            }
            config.lang = argv[++i];
            I18n::instance().setLang(config.lang);
        } else if (IsFlag(argc, argv, i, "-h", "--help")) {
            PrintUsage(argv[0]);
            std::exit(0);
        } else if (IsFlag(argc, argv, i, "-v", "--version")) {
            PrintVersion();
            std::exit(0);
        } else if (IsFlag(argc, argv, i, "-o", "--output")) {
            // 指定输出目录
            if (!HasArg(argc, argv, i)) {
                LOG_ERROR("--output requires a path argument");
                return false;
            }
            config.outputDir = fs::absolute(argv[++i]);
        } else if (IsFlag(argc, argv, i, "", "--app-name")) {
            // 指定应用名称
            if (!HasArg(argc, argv, i)) {
                LOG_ERROR("--app-name requires an argument");
                return false;
            }
            config.appName = argv[++i];
        } else if (IsFlag(argc, argv, i, "", "--jar")) {
            // 指定本地 JAR 路径（跳过 GitHub 下载）
            if (!HasArg(argc, argv, i)) {
                LOG_ERROR("--jar requires a path argument");
                return false;
            }
            config.jarPath = argv[++i];
        } else if (IsFlag(argc, argv, i, "", "--tag")) {
            // 指定 HMCL 版本标签
            if (!HasArg(argc, argv, i)) {
                LOG_ERROR("--tag requires an argument");
                return false;
            }
            config.tag = argv[++i];
        } else if (IsFlag(argc, argv, i, "", "--no-dmg")) {
            // 跳过 DMG 创建
            config.noDmg = true;
        } else if (IsFlag(argc, argv, i, "", "--bundle-java")) {
            config.bundleJava = true;
        } else if (IsFlag(argc, argv, i, "", "--java-path")) {
            // 指定 Java 路径，同时隐含 --bundle-java
            if (!HasArg(argc, argv, i)) {
                LOG_ERROR("--java-path requires a path argument");
                return false;
            }
            config.javaPath = argv[++i];
            config.bundleJava = true;
        } else if (IsFlag(argc, argv, i, "", "--skip-icon")) {
            config.skipIcon = true;
        } else if (IsFlag(argc, argv, i, "", "--clean")) {
            config.clean = true;
        } else if (IsFlag(argc, argv, i, "", "--keep-temp")) {
            config.keepTemp = true;
        } else if (IsFlag(argc, argv, i, "", "--proxy")) {
            // 指定 GitHub 下载代理 URL
            if (!HasArg(argc, argv, i)) {
                LOG_ERROR("--proxy requires a URL argument");
                return false;
            }
            config.proxyUrl = argv[++i];
        } else if (IsFlag(argc, argv, i, "", "--skip-licenses")) {
            config.skipLicenses = true;
        } else if (IsFlag(argc, argv, i, "", "--verbose")) {
            config.verbose = true;
        } else {
            LOG_ERROR("Unknown option: {}", argv[i]);
            return false;
        }
    }
    return true;
}

// 打印程序帮助信息（调用 i18n 获取对应语言的用法文本）
void PrintUsage(const char* programName) {
    std::cout << I18n::instance().t("usage_text", programName);
}

// 打印程序版本号
void PrintVersion() {
    std::cout << I18n::instance().t("hmcl-mac-builder version {}\n", HMCL_MAC_BUILDER_VERSION);
}
