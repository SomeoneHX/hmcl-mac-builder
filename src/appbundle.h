#pragma once

#include "config.h"
#include <string>
#include <filesystem>
namespace fs = std::filesystem;

bool CreateAppBundle(const Config& config, const fs::path& jarPath,
                     const fs::path& icnsPath, const fs::path& launcherPath,
                     const std::string& version, const BuildInfo& buildInfo,
                     bool verbose, const fs::path& javaHome = "");
