// network.h — 网络请求声明（GitHub API 交互、文件下载）
#pragma once

#include <string>
#include <filesystem>
namespace fs = std::filesystem;

// ReleaseInfo: GitHub Release 的解析结果
struct ReleaseInfo {
    std::string version;  // 版本标签（如 "3.5.7"）
    std::string jarUrl;   // JAR 文件下载链接
    bool valid = false;   // 是否成功获取到有效的发布信息
};

// 获取 HMCL 的最新 Release 信息（指定 tag 则获取特定版本）
ReleaseInfo GetLatestRelease(const std::string& tag = "");
// HTTP GET 请求，返回响应体字符串
std::string HttpGet(const std::string& url);
// 下载文件到本地路径，支持代理
bool DownloadFile(const std::string& url, const fs::path& dest, const std::string& proxyUrl = "");
// 返回 HMCL 官方图标的下载 URL
std::string GetIconUrl();
