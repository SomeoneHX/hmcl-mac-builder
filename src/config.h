#pragma once

#include <string>
#include <filesystem>
namespace fs = std::filesystem;

struct Config {
    fs::path outputDir = fs::current_path() / "output";
    std::string appName = "HMCL";
    std::string jarPath;
    std::string tag;
    bool noDmg = false;
    bool skipIcon = false;
    bool clean = false;
    bool keepTemp = false;
    bool verbose = false;
};

bool ParseArgs(int argc, char* argv[], Config& config);
void PrintUsage(const char* programName);
void PrintVersion();
