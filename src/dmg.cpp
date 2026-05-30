#include "dmg.h"
#include "utils.h"
#include <iostream>
#include <sstream>

bool CreateDMG(const Config& config, const std::string& version, bool verbose) {
    if (!Which("create-dmg")) {
        LOG_WARNING("create-dmg not found. Install with: brew install create-dmg");
        LOG_WARNING("Skipping DMG creation.");
        return false;
    }

    fs::path appPath = config.outputDir / (config.appName + ".app");
    fs::path dmgPath = config.outputDir / (config.appName + "-" + version + ".dmg");

    if (!fs::exists(appPath)) {
        LOG_ERROR("App bundle not found at " << appPath);
        return false;
    }

    std::ostringstream cmd;
    cmd << "create-dmg";
    cmd << " --volname \"" << config.appName << " Installer\"";
    cmd << " --window-size 660 400";
    cmd << " --icon-size 100";
    cmd << " --icon \"" << config.appName << ".app\" 160 185";
    cmd << " --app-drop-link 500 185";
    cmd << " \"" << dmgPath.string() << "\"";
    cmd << " \"" << appPath.string() << "\"";
    cmd << " >/dev/null";

    LOG_VERBOSE("Running: create-dmg for " << dmgPath, verbose);

    int rc = RunCommand(cmd.str());
    if (rc != 0) {
        LOG_ERROR("create-dmg failed with exit code " << rc);
        return false;
    }

    LOG_INFO("DMG created at " << dmgPath);
    return true;
}
