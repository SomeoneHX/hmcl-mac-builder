// utils.h — 通用工具函数声明：命令执行、文件读写、临时目录
#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include "i18n.h"
namespace fs = std::filesystem;

// 使用 system() 运行命令，返回退出码
int RunCommand(const std::string& cmd);
// 运行命令并捕获 stdout 输出（通过 popen），返回是否成功
bool RunCommandCapture(const std::string& cmd, std::string& output);
// 检查系统是否存在指定命令（通过 which）
bool Which(const std::string& program);
// 将字符串写入文件（覆盖），成功返回 true
bool WriteFile(const fs::path& path, const std::string& content);
// 读取文件全部内容到字符串，成功返回 true
bool ReadFile(const fs::path& path, std::string& content);
// 去除字符串首尾空白字符
std::string Trim(const std::string& s);
// 执行命令并返回去除首尾空白的输出结果
std::string CaptureOutput(const std::string& cmd);

// TempDir: RAII 临时目录管理类，析构时自动清理
class TempDir {
public:
    TempDir();
    ~TempDir();
    TempDir(const TempDir&) = delete;
    TempDir& operator=(const TempDir&) = delete;
    const fs::path& path() const { return path_; }
    fs::path path() { return path_; }
    // 释放临时目录所有权（析构时不再自动删除）
    void release();

private:
    fs::path path_;
    bool released_ = false;
};
