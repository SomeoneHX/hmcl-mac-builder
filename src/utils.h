#pragma once

#include <string>
#include <vector>
#include <filesystem>
namespace fs = std::filesystem;

#define LOG_INFO(msg) do { std::cout << "[INFO] " << msg << std::endl; } while(0)
#define LOG_WARNING(msg) do { std::cerr << "[WARNING] " << msg << std::endl; } while(0)
#define LOG_ERROR(msg) do { std::cerr << "[ERROR] " << msg << std::endl; } while(0)
#define LOG_VERBOSE(msg, verbose) do { if ((verbose)) std::cout << "[VERBOSE] " << msg << std::endl; } while(0)

int RunCommand(const std::string& cmd);
bool RunCommandCapture(const std::string& cmd, std::string& output);
bool Which(const std::string& program);
bool WriteFile(const fs::path& path, const std::string& content);
bool ReadFile(const fs::path& path, std::string& content);

class TempDir {
public:
    TempDir();
    ~TempDir();
    TempDir(const TempDir&) = delete;
    TempDir& operator=(const TempDir&) = delete;
    const fs::path& path() const { return path_; }
    fs::path path() { return path_; }
    void release();

private:
    fs::path path_;
    bool released_ = false;
};
