#pragma once

#include <string>
#include <filesystem>
namespace fs = std::filesystem;

bool GenerateLauncherScript(const fs::path& outputPath, const std::string& appName, const std::string& version);
