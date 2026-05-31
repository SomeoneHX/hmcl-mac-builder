#pragma once

#include <string>
#include <filesystem>
namespace fs = std::filesystem;

struct ReleaseInfo {
    std::string version;
    std::string jarUrl;
    bool valid = false;
};

ReleaseInfo GetLatestRelease(const std::string& tag = "");
std::string HttpGet(const std::string& url);
bool DownloadFile(const std::string& url, const fs::path& dest, const std::string& proxyUrl = "");
std::string GetIconUrl();
