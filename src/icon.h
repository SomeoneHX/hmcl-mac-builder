#pragma once

#include <string>
#include <filesystem>
namespace fs = std::filesystem;

bool ProcessIcon(const fs::path& outputPath, bool verbose);
