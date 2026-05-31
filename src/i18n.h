#pragma once

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <iostream>
#include <cstdlib>

class I18n {
public:
    static I18n& instance();

    void setLang(const std::string& lang);
    const std::string& currentLang() const { return currentLang_; }

    std::string t(const std::string& key) const;

    template<typename... Args>
    std::string t(const std::string& key, Args&&... args) const {
        std::string fmt = t(key);
        std::vector<std::string> strArgs = {toString(std::forward<Args>(args))...};
        return format(fmt, strArgs);
    }

    static std::string detectLang();

private:
    I18n() = default;

    template<typename T>
    std::string toString(T&& val) const {
        std::ostringstream ss;
        ss << std::forward<T>(val);
        return ss.str();
    }

    std::string format(const std::string& fmt, const std::vector<std::string>& args) const;

    std::string currentLang_ = "en";
    std::map<std::string, std::string> strings_;
};

namespace Color {
    constexpr auto Reset  = "\033[0m";
    constexpr auto Red    = "\033[31m";
    constexpr auto Green  = "\033[32m";
    constexpr auto Yellow = "\033[33m";
    constexpr auto Cyan   = "\033[36m";
}

inline void logInfo(const std::string& msg) {
    std::cout << Color::Green << "[INFO] " << Color::Reset << msg << std::endl;
}

inline void logWarning(const std::string& msg) {
    std::cerr << Color::Yellow << "[WARNING] " << Color::Reset << msg << std::endl;
}

inline void logError(const std::string& msg) {
    std::cerr << Color::Red << "[ERROR] " << Color::Reset << msg << std::endl;
}

#define LOG_INFO(fmt, ...) do { \
    ::logInfo(I18n::instance().t(fmt, ##__VA_ARGS__)); \
} while(0)

#define LOG_WARNING(fmt, ...) do { \
    ::logWarning(I18n::instance().t(fmt, ##__VA_ARGS__)); \
} while(0)

#define LOG_ERROR(fmt, ...) do { \
    ::logError(I18n::instance().t(fmt, ##__VA_ARGS__)); \
} while(0)

#define LOG_VERBOSE(msg, verbose) do { \
    if ((verbose)) std::cout << Color::Cyan << "[VERBOSE] " << Color::Reset << msg << std::endl; \
} while(0)
