// network.cpp — 基于 libcurl 的网络请求实现（GitHub API 调用、文件下载）
#include "network.h"
#include "version.h"
#include "utils.h"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <iostream>

using json = nlohmann::json;

// curl 写回调：将响应数据追加到字符串中
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t total = size * nmemb;
    output->append(static_cast<char*>(contents), total);
    return total;
}

// curl 写回调：将响应数据直接写入文件
static size_t FileWriteCallback(void* ptr, size_t size, size_t nmemb, FILE* stream) {
    return fwrite(ptr, size, nmemb, stream);
}

// 判断 URL 是否指向 GitHub 域名（用于决定是否应用代理）
static bool IsGithubUrl(const std::string& url) {
    return url.find("github.com") != std::string::npos
        || url.find("githubusercontent.com") != std::string::npos;
}

// 如果启用了代理且 URL 指向 GitHub，则在 URL 前拼接代理前缀
static std::string ApplyProxy(const std::string& url, const std::string& proxyUrl) {
    if (!proxyUrl.empty() && IsGithubUrl(url)) {
        return proxyUrl + url;
    }
    return url;
}

// 执行 curl GET 请求并返回响应字符串，支持重定向追踪
static std::string CurlGet(const std::string& url, bool followRedirect = true) {
    CURL* curl = curl_easy_init();
    if (!curl) return "";

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "hmcl-mac-builder/" HMCL_MAC_BUILDER_VERSION);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, followRedirect ? 1L : 0L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        LOG_ERROR("curl request failed: {} ({})", curl_easy_strerror(res), url);
        response.clear();
    }

    curl_easy_cleanup(curl);
    return response;
}

// 公开的 HTTP GET 封装
std::string HttpGet(const std::string& url) {
    return CurlGet(url);
}

// 下载文件到指定路径，支持代理；验证 HTTP 状态码是否为 200
bool DownloadFile(const std::string& url, const fs::path& dest, const std::string& proxyUrl) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        LOG_ERROR("Failed to initialize curl");
        return false;
    }

    FILE* fp = fopen(dest.c_str(), "wb");
    if (!fp) {
        LOG_ERROR("Failed to open file for writing: {}", dest);
        curl_easy_cleanup(curl);
        return false;
    }

    std::string effectiveUrl = ApplyProxy(url, proxyUrl);
    curl_easy_setopt(curl, CURLOPT_URL, effectiveUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, FileWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "hmcl-mac-builder/" HMCL_MAC_BUILDER_VERSION);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 120L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);

    CURLcode res = curl_easy_perform(curl);
    fclose(fp);

    if (res != CURLE_OK) {
        LOG_ERROR("Download failed: {} ({})", curl_easy_strerror(res), effectiveUrl);
        curl_easy_cleanup(curl);
        fs::remove(dest);
        return false;
    }

    long statusCode;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &statusCode);
    curl_easy_cleanup(curl);

    if (statusCode != 200) {
        LOG_ERROR("Download returned HTTP {} ({})", statusCode, effectiveUrl);
        fs::remove(dest);
        return false;
    }

    return true;
}

// 调用 GitHub API 获取 HMCL 最新 Release 信息，解析出版本号和 JAR 下载链接
ReleaseInfo GetLatestRelease(const std::string& tag) {
    ReleaseInfo info;

    std::string url;
    if (tag.empty()) {
        // 获取最新稳定版
        url = "https://api.github.com/repos/HMCL-dev/HMCL/releases/latest";
    } else {
        // 获取指定标签的版本
        url = "https://api.github.com/repos/HMCL-dev/HMCL/releases/tags/" + tag;
    }

    std::string response = CurlGet(url);
    if (response.empty()) {
        LOG_ERROR("Failed to fetch release info from GitHub API");
        return info;
    }

    try {
        json data = json::parse(response);

        // GitHub API 返回 "message": "Not Found" 表示版本不存在
        if (data.contains("message") && data["message"] == "Not Found") {
            LOG_ERROR("Release not found. Check the tag name.");
            return info;
        }

        info.version = data.value("tag_name", "unknown");
        info.valid = true;

        // 遍历 assets 列表，找到第一个 JAR 文件
        if (data.contains("assets") && data["assets"].is_array()) {
            for (auto& asset : data["assets"]) {
                std::string name = asset.value("name", "");
                if (name.find(".jar") != std::string::npos) {
                    info.jarUrl = asset.value("browser_download_url", "");
                    break;
                }
            }
        }
    } catch (const json::exception& e) {
        LOG_ERROR("Failed to parse GitHub API response: {}", e.what());
        info.valid = false;
    }

    return info;
}

// 返回 HMCL 官方 macOS 图标的原始 URL
std::string GetIconUrl() {
    return "https://raw.githubusercontent.com/HMCL-dev/HMCL/main/HMCL/src/main/resources/assets/img/icon-mac.png";
}
