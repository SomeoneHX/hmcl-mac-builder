#include "utils.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>

int RunCommand(const std::string& cmd) {
    return std::system(cmd.c_str());
}

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

bool Which(const std::string& program) {
    std::string out;
    std::string cmd = "which " + program + " 2>/dev/null";
    return RunCommandCapture(cmd, out) && !out.empty();
}

bool WriteFile(const fs::path& path, const std::string& content) {
    std::ofstream ofs(path);
    if (!ofs) return false;
    ofs << content;
    return ofs.good();
}

bool ReadFile(const fs::path& path, std::string& content) {
    std::ifstream ifs(path);
    if (!ifs) return false;
    std::ostringstream ss;
    ss << ifs.rdbuf();
    content = ss.str();
    return ifs.good();
}

std::string Trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\n\r\f\v");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\n\r\f\v");
    return s.substr(start, end - start + 1);
}

std::string CaptureOutput(const std::string& cmd) {
    std::string out;
    RunCommandCapture(cmd, out);
    return Trim(out);
}

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

TempDir::~TempDir() {
    if (!released_ && !path_.empty()) {
        fs::remove_all(path_);
    }
}

void TempDir::release() {
    released_ = true;
}
