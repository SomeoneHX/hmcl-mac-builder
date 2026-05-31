#pragma once

#include <string>
#include <filesystem>
namespace fs = std::filesystem;

struct BuildInfo {
    std::string builderVersion;
    std::string builderName;
    std::string builderRepo;
    std::string buildDate;
    std::string buildTime;
    std::string buildArgs;
    std::string macOSVersion;
    std::string macOSBuild;
    std::string architecture;
    std::string compilerVersion;
    std::string cmakeVersion;
    std::string xcodeVersion;
    std::string javaVersion;
    std::string javaHome;
    std::string createDmgVersion;
};

struct Config {
    fs::path outputDir = fs::current_path() / "output";
    std::string appName = "HMCL";
    std::string jarPath;
    std::string tag;
    std::string proxyUrl;
    std::string lang;
    bool bundleJava = false;
    std::string javaPath;
    bool noDmg = false;
    bool skipIcon = false;
    bool clean = false;
    bool keepTemp = false;
    bool verbose = false;
};

bool ParseArgs(int argc, char* argv[], Config& config);
void PrintUsage(const char* programName);
void PrintVersion();
