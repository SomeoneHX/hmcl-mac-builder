#include "launcher.h"
#include "utils.h"
#include <sstream>
#include <iostream>

bool GenerateLauncherScript(const fs::path& outputPath, const std::string& appName,
                            const std::string& version, const BuildInfo& buildInfo,
                            bool bundleJava) {
    std::ostringstream script;
    script << "#!/bin/bash\n";
    script << "# ==========================================\n";
    script << "# " << buildInfo.builderName << "\n";
    script << "# 仓库: " << buildInfo.builderRepo << "\n";
    script << "# 构建工具版本: " << buildInfo.builderVersion << "\n";
    script << "# 构建日期: " << buildInfo.buildDate << " " << buildInfo.buildTime << "\n";
    script << "# 构建参数:" << buildInfo.buildArgs << "\n";
    if (!buildInfo.macOSVersion.empty()) {
        script << "# 系统版本: macOS " << buildInfo.macOSVersion;
        if (!buildInfo.macOSBuild.empty()) {
            script << " (" << buildInfo.macOSBuild << ")";
        }
        if (!buildInfo.architecture.empty()) {
            script << " " << buildInfo.architecture;
        }
        script << "\n";
    }
    if (!buildInfo.compilerVersion.empty()) {
        script << "# C++ 编译器: " << buildInfo.compilerVersion << "\n";
    }
    if (!buildInfo.cmakeVersion.empty()) {
        script << "# CMake: " << buildInfo.cmakeVersion << "\n";
    }
    if (!buildInfo.xcodeVersion.empty()) {
        script << "# Xcode: " << buildInfo.xcodeVersion << "\n";
    }
    if (!buildInfo.javaVersion.empty()) {
        script << "# Java 版本: " << buildInfo.javaVersion << "\n";
        script << "# Java 路径: " << buildInfo.javaHome << "\n";
    }
    if (!buildInfo.createDmgVersion.empty()) {
        script << "# create-dmg: " << buildInfo.createDmgVersion << "\n";
    }
    script << "# ==========================================\n";
    script << "# " << appName << " Launcher\n";
    script << "# Version: " << version << "\n";
    script << "# Build: " << buildInfo.buildDate << "\n";
    script << "DIR=\"$(cd \"$(dirname \"$0\")\" && pwd)\"\n";
    script << "JAR=\"$DIR/../Resources/" << appName << ".jar\"\n";
    script << "if [ ! -f \"$JAR\" ]; then\n";
    script << "  osascript -e 'display dialog \"找不到 " << appName << " 主程序，应用可能已损坏\" buttons {\"确定\"} default button \"确定\" with title \"" << appName << "\" with icon stop'\n";
    script << "  exit 1\n";
    script << "fi\n";
    script << "cd \"$HOME\"\n";
    if (bundleJava) {
        script << "JAVA=\"$DIR/../Java/bin/java\"\n";
        script << "if [ ! -x \"$JAVA\" ]; then\n";
        script << "  osascript -e 'display dialog \"Java 运行环境丢失，应用可能已损坏\" buttons {\"确定\"} default button \"确定\" with title \"" << appName << "\" with icon stop'\n";
        script << "  exit 1\n";
        script << "fi\n";
        script << "exec \"$JAVA\" -jar \"$JAR\"\n";
    } else {
        script << "if [ ! -x /usr/bin/java ]; then\n";
        script << "  osascript -e 'display dialog \"无法找到 Java 运行环境，请确保 /usr/bin/java 能够正确运行\" buttons {\"确定\"} default button \"确定\" with title \"" << appName << "\" with icon stop'\n";
        script << "  exit 1\n";
        script << "fi\n";
        script << "exec /usr/bin/java -jar \"$JAR\"\n";
    }

    if (!WriteFile(outputPath, script.str())) {
        LOG_ERROR("Failed to write launcher script to {}", outputPath);
        return false;
    }

    fs::permissions(outputPath,
        fs::perms::owner_all | fs::perms::group_read | fs::perms::group_exec |
        fs::perms::others_read | fs::perms::others_exec);

    return true;
}
