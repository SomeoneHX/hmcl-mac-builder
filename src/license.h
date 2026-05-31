#pragma once

#include <string>
#include <filesystem>
namespace fs = std::filesystem;

struct LicenseInfo {
    fs::path licensesDir;
    bool hmclIncluded = false;
    bool openJdkIncluded = false;
    bool valid() const { return hmclIncluded; }
};

LicenseInfo DownloadLicenses(const fs::path& tempDir, bool bundleOpenJdk,
                             const std::string& javaHome,
                             const std::string& proxyUrl);
