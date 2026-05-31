// utils.cpp — 通用工具函数实现：命令执行、文件读写、临时目录
#include "utils.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>

// 使用 system() 执行命令并返回退出码
int RunCommand(const std::string& cmd) {
    return std::system(cmd.c_str());
}

// 使用 popen 执行命令，将 stdout 捕获到 output 字符串，返回命令是否成功退出
bool RunCommandCapture(const std::string& cmd, std::string& output) {
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return false;
    char buffer[4096];
    std::ostringstream ss;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        ss << buffer;
    }
    int rc = pclose(pipe);
    output = ss.str();
    return rc == 0;
}

// 检查系统中是否存在指定命令（通过 which 查找）
bool Which(const std::string& program) {
    std::string out;
    std::string cmd = "which " + program + " 2>/dev/null";
    return RunCommandCapture(cmd, out) && !out.empty();
}

// 将字符串内容写入文件（覆盖模式）
bool WriteFile(const fs::path& path, const std::string& content) {
    std::ofstream ofs(path);
    if (!ofs) return false;
    ofs << content;
    return ofs.good();
}

// 读取文件全部内容到字符串
bool ReadFile(const fs::path& path, std::string& content) {
    std::ifstream ifs(path);
    if (!ifs) return false;
    std::ostringstream ss;
    ss << ifs.rdbuf();
    content = ss.str();
    return ifs.good();
}

// 去除字符串首尾的空白字符（空格、制表符、换行符等）
std::string Trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\n\r\f\v");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\n\r\f\v");
    return s.substr(start, end - start + 1);
}

// 执行命令并返回去除首尾空白的输出
std::string CaptureOutput(const std::string& cmd) {
    std::string out;
    RunCommandCapture(cmd, out);
    return Trim(out);
}

// 从 JAR 文件路径中提取 HMCL 版本号
// 支持文件名格式如 HMCL-3.5.7.jar、HMCL-3.5.227.jar 等
std::string ExtractVersionFromJarName(const std::string& jarPath) {
    fs::path p(jarPath);
    std::string filename = p.filename().string();

    const std::string prefix = "HMCL-";
    const std::string suffix = ".jar";

    if (filename.size() <= prefix.size() + suffix.size()) return "";
    if (filename.substr(0, prefix.size()) != prefix) return "";
    if (filename.substr(filename.size() - suffix.size()) != suffix) return "";

    std::string version = filename.substr(
        prefix.size(),
        filename.size() - prefix.size() - suffix.size()
    );

    if (version.empty() || !std::isdigit(static_cast<unsigned char>(version[0]))) return "";
    for (char c : version) {
        if (!std::isdigit(static_cast<unsigned char>(c)) && c != '.') return "";
    }

    return version;
}

// 转义 shell 命令参数中的特殊字符，防止命令注入
std::string EscapeShellArg(const std::string& arg) {
    std::string result;
    for (char c : arg) {
        if (c == '"' || c == '\\' || c == '`' || c == '$') {
            result += '\\';
        }
        result += c;
    }
    return result;
}

// 使用 mkdtemp 创建唯一临时目录
TempDir::TempDir() {
    std::string pattern = "/tmp/hmcl-build-XXXXXX";
    char* tmpl = strdup(pattern.c_str());
    char* result = mkdtemp(tmpl);
    if (result) {
        path_ = fs::path(result);
    } else {
        throw std::runtime_error("Failed to create temporary directory");
    }
    free(tmpl);
}

// 析构时自动删除临时目录（除非已调用 release()）
TempDir::~TempDir() {
    if (!released_ && !path_.empty()) {
        std::error_code ec;
        fs::remove_all(path_, ec);
    }
}

// 释放临时目录所有权，析构时不再自动删除
void TempDir::release() {
    released_ = true;
}
