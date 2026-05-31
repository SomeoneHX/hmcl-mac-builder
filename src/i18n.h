// i18n.h — 国际化（i18n）支持及彩色日志输出
#pragma once

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <iostream>
#include <cstdlib>

// I18n: 单例模式的国际化支持类，管理中英文字符串翻译
class I18n {
public:
    static I18n& instance();

    void setLang(const std::string& lang);
    const std::string& currentLang() const { return currentLang_; }

    // 根据键获取翻译字符串（无参数版本）
    std::string t(const std::string& key) const;

    // 根据键获取翻译字符串，并用可变参数替换 {} 占位符
    template<typename... Args>
    std::string t(const std::string& key, Args&&... args) const {
        std::string fmt = t(key);
        std::vector<std::string> strArgs = {toString(std::forward<Args>(args))...};
        return format(fmt, strArgs);
    }

    // 根据 LANG 环境变量自动检测语言
    static std::string detectLang();

private:
    I18n() = default;

    // 将任意类型转为字符串
    template<typename T>
    std::string toString(T&& val) const {
        std::ostringstream ss;
        ss << std::forward<T>(val);
        return ss.str();
    }

    // 用参数列表替换字符串中的 {} 占位符
    std::string format(const std::string& fmt, const std::vector<std::string>& args) const;

    std::string currentLang_ = "en";       // 当前语言（默认英文）
    std::map<std::string, std::string> strings_; // 字符串映射表
};

// ANSI 转义码颜色常量，用于终端彩色输出
namespace Color {
    constexpr auto Reset  = "\033[0m";
    constexpr auto Red    = "\033[31m";
    constexpr auto Green  = "\033[32m";
    constexpr auto Yellow = "\033[33m";
    constexpr auto Cyan   = "\033[36m";
}

// 输出绿色 [INFO] 信息到 stdout
inline void logInfo(const std::string& msg) {
    std::cout << Color::Green << "[INFO] " << Color::Reset << msg << std::endl;
}

// 输出黄色 [WARNING] 警告到 stderr
inline void logWarning(const std::string& msg) {
    std::cerr << Color::Yellow << "[WARNING] " << Color::Reset << msg << std::endl;
}

// 输出红色 [ERROR] 错误到 stderr
inline void logError(const std::string& msg) {
    std::cerr << Color::Red << "[ERROR] " << Color::Reset << msg << std::endl;
}

// 宏：带 i18n 翻译的 INFO 级别日志
#define LOG_INFO(fmt, ...) do { \
    ::logInfo(I18n::instance().t(fmt, ##__VA_ARGS__)); \
} while(0)

// 宏：带 i18n 翻译的 WARNING 级别日志
#define LOG_WARNING(fmt, ...) do { \
    ::logWarning(I18n::instance().t(fmt, ##__VA_ARGS__)); \
} while(0)

// 宏：带 i18n 翻译的 ERROR 级别日志
#define LOG_ERROR(fmt, ...) do { \
    ::logError(I18n::instance().t(fmt, ##__VA_ARGS__)); \
} while(0)

// 宏：仅在 verbose 模式下输出的青色 [VERBOSE] 日志（不经过 i18n）
#define LOG_VERBOSE(msg, verbose) do { \
    if ((verbose)) std::cout << Color::Cyan << "[VERBOSE] " << Color::Reset << msg << std::endl; \
} while(0)
