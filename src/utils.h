#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include "i18n.h"
namespace fs = std::filesystem;

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
