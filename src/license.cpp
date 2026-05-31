#include "license.h"
#include "network.h"
#include "utils.h"
#include <fstream>
#include <iostream>

static const std::string HMCL_LICENSE_URL =
    "https://raw.githubusercontent.com/HMCL-dev/HMCL/main/LICENSE";
static const std::string OPENJDK_LICENSE_URL =
    "https://raw.githubusercontent.com/openjdk/jdk/master/LICENSE";
static const std::string OPENJDK_ASSEMBLY_EXCEPTION_URL =
    "https://raw.githubusercontent.com/openjdk/jdk/master/ASSEMBLY_EXCEPTION";

LicenseInfo DownloadLicenses(const fs::path& tempDir, bool bundleOpenJdk,
                             const std::string& javaHome,
                             const std::string& proxyUrl) {
    LicenseInfo info;
    fs::path licensesDir = tempDir / "licenses";
    fs::create_directories(licensesDir);
    info.licensesDir = licensesDir;

    fs::path hmclDest = licensesDir / "HMCL-LICENSE";
    LOG_INFO("Downloading HMCL license...");
    if (DownloadFile(HMCL_LICENSE_URL, hmclDest, proxyUrl)) {
        info.hmclIncluded = true;
        LOG_INFO("HMCL license downloaded to {}", hmclDest);
    } else {
        LOG_WARNING("Failed to download HMCL license, continuing without it.");
    }

    if (bundleOpenJdk && !javaHome.empty()) {
        fs::path jdkLicensePath = fs::path(javaHome) / "LICENSE";
        fs::path jdkAssemblyException = fs::path(javaHome) / "ASSEMBLY_EXCEPTION";
        fs::path openJdkDest = licensesDir / "OPENJDK-LICENSE";

        if (fs::exists(jdkLicensePath)) {
            try {
                fs::copy_file(jdkLicensePath, openJdkDest, fs::copy_options::overwrite_existing);
                info.openJdkIncluded = true;
                LOG_INFO("OpenJDK license copied from {}", jdkLicensePath);
            } catch (const fs::filesystem_error& e) {
                LOG_WARNING("Failed to copy OpenJDK license: {}", e.what());
            }
        } else {
            LOG_INFO("OpenJDK LICENSE not found locally, downloading from GitHub...");
            if (DownloadFile(OPENJDK_LICENSE_URL, openJdkDest, proxyUrl)) {
                info.openJdkIncluded = true;
                LOG_INFO("OpenJDK license downloaded from GitHub");
            } else {
                LOG_WARNING("Failed to download OpenJDK license from GitHub");
            }
        }

        if (fs::exists(jdkAssemblyException)) {
            try {
                fs::path assemblyDest = licensesDir / "ASSEMBLY_EXCEPTION";
                fs::copy_file(jdkAssemblyException, assemblyDest, fs::copy_options::overwrite_existing);
            } catch (const fs::filesystem_error&) {}
        } else {
            fs::path assemblyDest = licensesDir / "ASSEMBLY_EXCEPTION";
            LOG_INFO("ASSEMBLY_EXCEPTION not found locally, downloading from GitHub...");
            if (!DownloadFile(OPENJDK_ASSEMBLY_EXCEPTION_URL, assemblyDest, proxyUrl)) {
                LOG_WARNING("Failed to download ASSEMBLY_EXCEPTION from GitHub");
            }
        }
    }

    return info;
}
