// launcher.cpp — 生成 launch.sh 脚本，包含构建元信息注释和 JAR 启动逻辑
#include "launcher.h"
#include "utils.h"
#include <sstream>
#include <iostream>

// 生成 .app 内部的启动脚本：解析 JAR 路径、检测 Java、启动 HMCL
bool GenerateLauncherScript(const fs::path& outputPath, const std::string& appName,
                            const std::string& version, const BuildInfo& buildInfo,
                            bool bundleJava) {
    std::ostringstream script;
    // Shell 脚本头部和构建元信息注释块
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
    script << "# " << appName << " Launcher\n";
    script << "# Version: " << version << "\n";
    script << "# ==========================================\n";

    // 运行时检测系统语言：优先 $LANG，回退 macOS 系统偏好，最后 en
    script << "if [ -z \"$LANG\" ]; then\n";
    script << "    LANG=$(defaults read -g AppleLocale 2>/dev/null)\n";
    script << "    [ -n \"$LANG\" ] && LANG=\"${LANG}.UTF-8\"\n";
    script << "    LANG=\"${LANG:-en}\"\n";
    script << "fi\n";
    script << "case \"${LANG:-en}\" in\n";
    script << "    zh*)\n";
    script << "        MSG_JAR=\"找不到 " << appName << " 主程序，应用可能已损坏\"\n";
    script << "        MSG_JAVA=\"Java 运行环境丢失，应用可能已损坏\"\n";
    script << "        MSG_SYS_JAVA=\"无法找到 Java 运行环境，请确保 /usr/bin/java 能够正确运行\"\n";
    script << "        BTN_OK=\"确定\"\n";
    script << "        ;;\n";
    script << "    *)\n";
    script << "        MSG_JAR=\"Cannot find " << appName << " main program. The application may be corrupted.\"\n";
    script << "        MSG_JAVA=\"Java runtime is missing. The application may be corrupted.\"\n";
    script << "        MSG_SYS_JAVA=\"Cannot find Java runtime. Please make sure /usr/bin/java works correctly.\"\n";
    script << "        BTN_OK=\"OK\"\n";
    script << "        ;;\n";
    script << "esac\n";

    script << "DIR=\"$(cd \"$(dirname \"$0\")\" && pwd)\"\n";
    script << "JAR=\"$DIR/../Resources/" << appName << ".jar\"\n";
    script << "if [ ! -f \"$JAR\" ]; then\n";
    script << "  osascript -e \"display dialog \\\"$MSG_JAR\\\" buttons {\\\"$BTN_OK\\\"} default button \\\"$BTN_OK\\\" with title \\\"" << appName << "\\\" with icon stop\"\n";
    script << "  exit 1\n";
    script << "fi\n";
    script << "cd \"$HOME\"\n";
    // 根据是否打包 Java 选择不同的启动路径
    if (bundleJava) {
        // 使用打包在 .app 内部的 Java 运行时
        script << "JAVA=\"$DIR/../Java/bin/java\"\n";
        script << "if [ ! -x \"$JAVA\" ]; then\n";
        script << "  osascript -e \"display dialog \\\"$MSG_JAVA\\\" buttons {\\\"$BTN_OK\\\"} default button \\\"$BTN_OK\\\" with title \\\"" << appName << "\\\" with icon stop\"\n";
        script << "  exit 1\n";
        script << "fi\n";
        script << "exec \"$JAVA\" -jar \"$JAR\"\n";
    } else {
        // 使用系统自带的 /usr/bin/java
        script << "if [ ! -x /usr/bin/java ]; then\n";
        script << "  osascript -e \"display dialog \\\"$MSG_SYS_JAVA\\\" buttons {\\\"$BTN_OK\\\"} default button \\\"$BTN_OK\\\" with title \\\"" << appName << "\\\" with icon stop\"\n";
        script << "  exit 1\n";
        script << "fi\n";
        script << "exec /usr/bin/java -jar \"$JAR\"\n";
    }

    // 写脚本文件
    if (!WriteFile(outputPath, script.str())) {
        LOG_ERROR("Failed to write launcher script to {}", outputPath);
        return false;
    }

    // 设置脚本可执行权限 (755)
    fs::permissions(outputPath,
        fs::perms::owner_all | fs::perms::group_read | fs::perms::group_exec |
        fs::perms::others_read | fs::perms::others_exec);

    return true;
}
