#include "network.h"
#include "utils.h"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <iostream>

using json = nlohmann::json;

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t total = size * nmemb;
    output->append(static_cast<char*>(contents), total);
    return total;
}

static size_t FileWriteCallback(void* ptr, size_t size, size_t nmemb, FILE* stream) {
    return fwrite(ptr, size, nmemb, stream);
}

static std::string CurlGet(const std::string& url, bool followRedirect = true) {
    CURL* curl = curl_easy_init();
    if (!curl) return "";

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "hmcl-mac-builder/1.0");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, followRedirect ? 1L : 0L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cerr << "[ERROR] curl request failed: " << curl_easy_strerror(res)
                  << " (" << url << ")" << std::endl;
        response.clear();
    }

    curl_easy_cleanup(curl);
    return response;
}

std::string HttpGet(const std::string& url) {
    return CurlGet(url);
}

bool DownloadFile(const std::string& url, const fs::path& dest) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        LOG_ERROR("Failed to initialize curl");
        return false;
    }

    FILE* fp = fopen(dest.c_str(), "wb");
    if (!fp) {
        LOG_ERROR("Failed to open file for writing: " << dest);
        curl_easy_cleanup(curl);
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, FileWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "hmcl-mac-builder/1.0");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 120L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);

    CURLcode res = curl_easy_perform(curl);
    fclose(fp);

    if (res != CURLE_OK) {
        LOG_ERROR("Download failed: " << curl_easy_strerror(res) << " (" << url << ")");
        curl_easy_cleanup(curl);
        fs::remove(dest);
        return false;
    }

    long statusCode;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &statusCode);
    curl_easy_cleanup(curl);

    if (statusCode != 200) {
        LOG_ERROR("Download returned HTTP " << statusCode << " (" << url << ")");
        fs::remove(dest);
        return false;
    }

    return true;
}

ReleaseInfo GetLatestRelease(const std::string& tag) {
    ReleaseInfo info;

    std::string url;
    if (tag.empty()) {
        url = "https://api.github.com/repos/HMCL-dev/HMCL/releases/latest";
    } else {
        url = "https://api.github.com/repos/HMCL-dev/HMCL/releases/tags/" + tag;
    }

    std::string response = CurlGet(url);
    if (response.empty()) {
        LOG_ERROR("Failed to fetch release info from GitHub API");
        return info;
    }

    try {
        json data = json::parse(response);

        if (data.contains("message") && data["message"] == "Not Found") {
            LOG_ERROR("Release not found. Check the tag name.");
            return info;
        }

        info.version = data.value("tag_name", "unknown");
        info.valid = true;

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
        LOG_ERROR("Failed to parse GitHub API response: " << e.what());
        info.valid = false;
    }

    return info;
}

std::string GetIconUrl() {
    return "https://raw.githubusercontent.com/HMCL-dev/HMCL/main/HMCL/src/main/resources/assets/img/icon-mac.png";
}
