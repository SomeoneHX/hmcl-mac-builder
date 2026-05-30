#include "launcher.h"
#include "utils.h"
#include <sstream>
#include <iostream>

bool GenerateLauncherScript(const fs::path& outputPath, const std::string& appName, const std::string& version) {
    std::ostringstream script;
    script << "#!/bin/bash\n";
    script << "DIR=\"$(cd \"$(dirname \"$0\")\" && pwd)\"\n";
    script << "JAR=\"$DIR/../Resources/" << appName << "-" << version << ".jar\"\n";
    script << "if [ ! -f \"$JAR\" ]; then\n";
    script << "  echo \"Error: HMCL JAR not found at $JAR\" >&2\n";
    script << "  exit 1\n";
    script << "fi\n";
    script << "cd \"$HOME\"\n";
    script << "exec /usr/bin/java -jar \"$JAR\"\n";

    if (!WriteFile(outputPath, script.str())) {
        LOG_ERROR("Failed to write launcher script to " << outputPath);
        return false;
    }

    fs::permissions(outputPath,
        fs::perms::owner_all | fs::perms::group_read | fs::perms::group_exec |
        fs::perms::others_read | fs::perms::others_exec);

    return true;
}
