// dmg.cpp — 使用 create-dmg 生成 macOS DMG 安装包
#include "dmg.h"
#include "utils.h"
#include <iostream>
#include <sstream>

// 调用 create-dmg 工具创建 DMG：设置卷名、窗口布局、应用图标和拖拽链接
bool CreateDMG(const Config& config, const std::string& version, bool verbose) {
    // 检查 create-dmg 是否已安装
    if (!Which("create-dmg")) {
        LOG_WARNING("create-dmg not found. Install with: brew install create-dmg");
        LOG_WARNING("Skipping DMG creation.");
        return false;
    }

    fs::path appPath = config.outputDir / (config.appName + ".app");
    fs::path dmgPath = config.outputDir / (config.appName + "-" + version + ".dmg");

    // 确认 .app 包已存在
    if (!fs::exists(appPath)) {
        LOG_ERROR("App bundle not found at {}", appPath);
        return false;
    }

    // 构造 create-dmg 命令参数
    std::ostringstream cmd;
    cmd << "create-dmg";
    cmd << " --volname \"" << config.appName << " Installer\"";   // DMG 卷标
    cmd << " --window-size 660 400";                                // 窗口尺寸
    cmd << " --icon-size 100";                                      // 图标大小
    cmd << " --icon \"" << config.appName << ".app\" 160 185";      // 应用图标位置
    cmd << " --app-drop-link 500 185";                               // Applications 拖拽链接
    cmd << " \"" << dmgPath.string() << "\"";                       // 输出路径
    cmd << " \"" << appPath.string() << "\"";                       // 输入 .app 路径
    cmd << " >/dev/null";                                           // 屏蔽标准输出

    LOG_VERBOSE("Running: create-dmg for " << dmgPath, verbose);

    int rc = RunCommand(cmd.str());
    if (rc != 0) {
        LOG_ERROR("create-dmg failed with exit code {}", rc);
        return false;
    }

    LOG_INFO("DMG created at {}", dmgPath);
    return true;
}
