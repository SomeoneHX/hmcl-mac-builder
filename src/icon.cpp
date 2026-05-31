#include "icon.h"
#include "network.h"
#include "utils.h"
#include <iostream>
#include <sstream>
#include <vector>

static const std::vector<std::pair<int, int>> kIconSizes = {
    {16, 16}, {16, 32}, {32, 32}, {32, 64},
    {64, 64}, {128, 128}, {256, 256}, {512, 512}, {1024, 1024}
};

static bool ConvertIcoToPng(const fs::path& icoPath, const fs::path& pngPath, bool verbose) {
    if (Which("magick")) {
        std::string cmd = "magick convert \"" + icoPath.string() + "\" \"" + pngPath.string() + "\"";
        LOG_VERBOSE("Running: " << cmd, verbose);
        if (RunCommand(cmd) == 0) return true;
    }

    if (Which("convert")) {
        std::string cmd = "convert \"" + icoPath.string() + "\" \"" + pngPath.string() + "\"";
        LOG_VERBOSE("Running: " << cmd, verbose);
        if (RunCommand(cmd) == 0) return true;
    }

    std::string sipsCmd = "sips -s format png \"" + icoPath.string() + "\" --out \"" + pngPath.string() + "\" >/dev/null 2>&1";
    LOG_VERBOSE("Running: " << sipsCmd, verbose);
    if (RunCommand(sipsCmd) == 0) return true;

    return false;
}

bool ProcessIcon(const fs::path& outputPath, bool verbose, const std::string& proxyUrl) {
    fs::path tempDir = outputPath.parent_path();
    fs::path icoPath = tempDir / "HMCL.ico";
    fs::path iconsetDir = tempDir / "HMCL.iconset";
    fs::path pngBase = tempDir / "HMCL.png";

    LOG_VERBOSE("Downloading icon from GitHub...", verbose);
    if (!DownloadFile(GetIconUrl(), icoPath, proxyUrl)) {
        LOG_ERROR("Failed to download icon file");
        return false;
    }
    LOG_VERBOSE("Icon downloaded to " << icoPath, verbose);

    LOG_VERBOSE("Converting ICO to PNG...", verbose);
    if (!ConvertIcoToPng(icoPath, pngBase, verbose)) {
        LOG_ERROR("Failed to convert ICO to PNG. Please install ImageMagick.");
        return false;
    }

    fs::create_directories(iconsetDir);
    LOG_VERBOSE("Creating iconset at " << iconsetDir, verbose);

    for (const auto& [size, scaleSize] : kIconSizes) {
        std::string filename;
        int scale = scaleSize / size;

        if (scale == 1) {
            filename = "icon_" + std::to_string(size) + "x" + std::to_string(size) + ".png";
        } else {
            filename = "icon_" + std::to_string(size) + "x" + std::to_string(size) + "@" + std::to_string(scale) + "x.png";
        }

        fs::path outPng = iconsetDir / filename;
        std::string cmd = "sips -z " + std::to_string(scaleSize) + " " +
                          std::to_string(scaleSize) + " \"" + pngBase.string() +
                          "\" --out \"" + outPng.string() + "\" >/dev/null 2>&1";
        LOG_VERBOSE("Running: " << cmd, verbose);
        if (RunCommand(cmd) != 0) {
            LOG_ERROR("Failed to resize icon to " << scaleSize << "x" << scaleSize);
            return false;
        }
    }

    LOG_VERBOSE("Generating ICNS with iconutil...", verbose);
    std::string iconutilCmd = "iconutil -c icns \"" + iconsetDir.string() +
                              "\" -o \"" + outputPath.string() + "\" 2>/dev/null";
    if (RunCommand(iconutilCmd) != 0) {
        LOG_ERROR("Failed to generate ICNS file");
        return false;
    }

    LOG_VERBOSE("ICNS generated at " << outputPath, verbose);
    return true;
}
