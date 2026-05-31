#pragma once

#include <string>
#include <filesystem>
namespace fs = std::filesystem;

struct JavaInfo {
    fs::path javaHome;
    std::string version;
    bool valid = false;
};

JavaInfo FindJava(const std::string& userPath = "");
bool BundleJava(const fs::path& javaHome, const fs::path& destDir, bool verbose);
