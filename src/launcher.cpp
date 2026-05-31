#include "launcher.h"
#include "utils.h"
#include <sstream>
#include <iostream>

bool GenerateLauncherScript(const fs::path& outputPath, const std::string& appName,
                            const std::string& version, const std::string& buildDate,
                            bool bundleJava) {
    std::ostringstream script;
    script << "#!/bin/bash\n";
    script << "# " << appName << " Launcher\n";
    script << "# Version: " << version << "\n";
    script << "# Build: " << buildDate << "\n";
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
