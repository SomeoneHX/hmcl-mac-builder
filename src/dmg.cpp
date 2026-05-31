// dmg.cpp — 使用 create-dmg 生成 macOS DMG 安装包
#include "dmg.h"
#include "utils.h"
#include <iostream>
#include <sstream>

// 调用 create-dmg 工具创建 DMG：设置卷名、窗口布局、应用图标和拖拽链接
bool CreateDMG(const Config& config, const std::string& version, bool verbose, const std::string& javaVersion, const std::string& javaArchitecture) {
    // 检查 create-dmg 是否已安装
    if (!Which("create-dmg")) {
        LOG_WARNING("create-dmg not found. Install with: brew install create-dmg");
        LOG_WARNING("Skipping DMG creation.");
        return false;
    }

    fs::path appPath = config.outputDir / (config.appName + ".app");
    std::string dmgName = config.appName + "-" + version;
    if (!javaVersion.empty()) {
        dmgName += "-java-" + javaVersion;
        if (!javaArchitecture.empty()) {
            dmgName += "-" + javaArchitecture;
        }
    }
    dmgName += ".dmg";
    fs::path dmgPath = config.outputDir / dmgName;

    // 确认 .app 包已存在
    if (!fs::exists(appPath)) {
        LOG_ERROR("App bundle not found at {}", appPath);
        return false;
    }

    // 构造 create-dmg 命令参数（escape 用户输入防注入）
    std::ostringstream cmd;
    cmd << "create-dmg";
    cmd << " --volname \"" << EscapeShellArg(config.appName) << " Installer\"";
    cmd << " --window-size 660 400";
    cmd << " --icon-size 100";
    cmd << " --icon \"" << EscapeShellArg(config.appName) << ".app\" 160 185";
    cmd << " --app-drop-link 500 185";
    cmd << " \"" << EscapeShellArg(dmgPath.string()) << "\"";
    cmd << " \"" << EscapeShellArg(appPath.string()) << "\"";

    LOG_VERBOSE("Running: create-dmg for " << dmgPath, verbose);

    int rc = RunCommand(cmd.str(), verbose);
    if (rc != 0) {
        LOG_ERROR("create-dmg failed with exit code {}", rc);
        return false;
    }

    LOG_INFO("DMG created at {}", dmgPath);
    return true;
}
