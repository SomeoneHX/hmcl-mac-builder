#pragma once

#include "config.h"
#include <string>

bool CreateDMG(const Config& config, const std::string& version, bool verbose,
               const std::string& javaVersion = "", const std::string& javaArchitecture = "");
