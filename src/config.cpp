#include "config.h"
#include "utils.h"
#include <iostream>
#include <cstring>
#include <cstdlib>

static bool IsFlag(int argc, char* argv[], int i, const std::string& shortFlag, const std::string& longFlag) {
    return strcmp(argv[i], shortFlag.c_str()) == 0 || strcmp(argv[i], longFlag.c_str()) == 0;
}

static bool HasArg(int argc, char* argv[], int i) {
    return i + 1 < argc;
}

bool ParseArgs(int argc, char* argv[], Config& config) {
    for (int i = 1; i < argc; i++) {
        if (IsFlag(argc, argv, i, "-h", "--help")) {
            PrintUsage(argv[0]);
            std::exit(0);
        } else if (IsFlag(argc, argv, i, "-v", "--version")) {
            PrintVersion();
            std::exit(0);
        } else if (IsFlag(argc, argv, i, "-o", "--output")) {
            if (!HasArg(argc, argv, i)) {
                LOG_ERROR("--output requires a path argument");
                return false;
            }
            config.outputDir = fs::absolute(argv[++i]);
        } else if (IsFlag(argc, argv, i, "", "--app-name")) {
            if (!HasArg(argc, argv, i)) {
                LOG_ERROR("--app-name requires an argument");
                return false;
            }
            config.appName = argv[++i];
        } else if (IsFlag(argc, argv, i, "", "--jar")) {
            if (!HasArg(argc, argv, i)) {
                LOG_ERROR("--jar requires a path argument");
                return false;
            }
            config.jarPath = argv[++i];
        } else if (IsFlag(argc, argv, i, "", "--tag")) {
            if (!HasArg(argc, argv, i)) {
                LOG_ERROR("--tag requires an argument");
                return false;
            }
            config.tag = argv[++i];
        } else if (IsFlag(argc, argv, i, "", "--no-dmg")) {
            config.noDmg = true;
        } else if (IsFlag(argc, argv, i, "", "--skip-icon")) {
            config.skipIcon = true;
        } else if (IsFlag(argc, argv, i, "", "--clean")) {
            config.clean = true;
        } else if (IsFlag(argc, argv, i, "", "--keep-temp")) {
            config.keepTemp = true;
        } else if (IsFlag(argc, argv, i, "", "--proxy")) {
            if (!HasArg(argc, argv, i)) {
                LOG_ERROR("--proxy requires a URL argument");
                return false;
            }
            config.proxyUrl = argv[++i];
        } else if (IsFlag(argc, argv, i, "", "--verbose")) {
            config.verbose = true;
        } else {
            LOG_ERROR("Unknown option: " << argv[i]);
            return false;
        }
    }
    return true;
}

void PrintUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [options]\n"
              << "Options:\n"
              << "  -h, --help            Show this help message\n"
              << "  -v, --version         Show version information\n"
              << "  -o, --output DIR      Output directory (default: ./output)\n"
              << "  --app-name NAME       Application name (default: HMCL)\n"
              << "  --jar PATH            Local JAR file path (default: download from GitHub)\n"
              << "  --tag VERSION         HMCL version tag (default: latest stable)\n"
              << "  --no-dmg              Only generate .app, skip DMG\n"
              << "  --skip-icon           Skip icon conversion\n"
              << "  --clean               Clean previous build files\n"
              << "  --proxy URL           Use proxy for GitHub requests (e.g. https://v4.gh-proxy.org/)\n"
              << "  --keep-temp           Keep temporary files\n"
              << "  --verbose             Verbose output\n";
}

void PrintVersion() {
    std::cout << "hmcl-mac-builder version 1.0.0\n";
}
