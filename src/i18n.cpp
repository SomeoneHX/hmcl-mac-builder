#include "i18n.h"
#include <algorithm>

static const std::string en_usage =
    "Usage: {} [options]\n"
    "Options:\n"
    "  -h, --help            Show this help message\n"
    "  -v, --version         Show version information\n"
    "  -o, --output DIR      Output directory (default: ./output)\n"
    "  --app-name NAME       Application name (default: HMCL)\n"
    "  --jar PATH            Local JAR file path (default: download from GitHub)\n"
    "  --tag VERSION         HMCL version tag (default: latest stable)\n"
    "  --no-dmg              Only generate .app, skip DMG\n"
    "  --skip-icon           Skip icon conversion\n"
    "  --clean               Clean previous build files\n"
    "  --proxy URL           Use proxy for GitHub requests (e.g. https://v4.gh-proxy.org/)\n"
    "  --keep-temp           Keep temporary files\n"
    "  --bundle-java         Auto-detect and bundle Java runtime into .app\n"
    "  --java-path PATH      Specify Java path to bundle (implies --bundle-java)\n"
    "  --lang zh|en          Output language (default: auto-detect)\n"
    "  --verbose             Verbose output\n";

static const std::string zh_usage =
    "用法: {} [选项]\n"
    "选项:\n"
    "  -h, --help            显示此帮助信息\n"
    "  -v, --version         显示版本信息\n"
    "  -o, --output DIR      输出目录（默认: ./output）\n"
    "  --app-name NAME       应用名称（默认: HMCL）\n"
    "  --jar PATH            本地 JAR 文件路径（默认: 从 GitHub 下载）\n"
    "  --tag VERSION         HMCL 版本标签（默认: 最新稳定版）\n"
    "  --no-dmg              仅生成 .app，跳过 DMG\n"
    "  --skip-icon           跳过图标转换\n"
    "  --clean               清理之前的构建文件\n"
    "  --proxy URL           GitHub 下载代理（如 https://v4.gh-proxy.org/）\n"
    "  --keep-temp           保留临时文件\n"
    "  --bundle-java         自动检测并打包 Java 运行时到 .app\n"
    "  --java-path PATH      指定要打包的 Java 路径（隐含 --bundle-java）\n"
    "  --lang zh|en          输出语言（默认: 自动检测）\n"
    "  --verbose             启用详细日志\n";

static const std::map<std::string, std::string> zh_strings = {
    {"Removed {}", "已删除 {}"},
    {"Nothing to clean in {}", "在 {} 中没有可清理的内容"},
    {"Using temporary directory: {}", "正在使用临时目录: {}"},
    {"Using local JAR: {}", "正在使用本地 JAR: {}"},
    {"Version set to: {}", "版本设置为: {}"},
    {"Fetching latest release info from GitHub...", "正在从 GitHub 获取最新发布信息..."},
    {"Failed to get release info. Use --jar to specify a local file.",
     "获取发布信息失败。使用 --jar 指定本地文件。"},
    {"No JAR file found in the release.", "在该发布中未找到 JAR 文件。"},
    {"Found version: {}", "找到版本: {}"},
    {"Downloading HMCL {}...", "正在下载 HMCL {}..."},
    {"Icon processing failed, proceeding without custom icon.",
     "图标处理失败，将继续而不使用自定义图标。"},
    {"Failed to download HMCL JAR", "下载 HMCL JAR 失败"},
    {"JAR downloaded successfully", "JAR 下载成功"},
    {"Generating launcher script...", "正在生成启动脚本..."},
    {"Failed to generate launcher script", "生成启动脚本失败"},
    {"Creating app bundle...", "正在创建应用捆绑包..."},
    {"Failed to create app bundle", "创建应用捆绑包失败"},
    {"Creating DMG...", "正在创建 DMG..."},
    {"Cleaning up temporary directory", "正在清理临时目录"},
    {"Temporary files kept at {}", "临时文件保留于 {}"},
    {"Done!", "完成！"},

    {"--output requires a path argument", "--output 需要路径参数"},
    {"--app-name requires an argument", "--app-name 需要参数"},
    {"--jar requires a path argument", "--jar 需要路径参数"},
    {"--tag requires an argument", "--tag 需要参数"},
    {"--proxy requires a URL argument", "--proxy 需要 URL 参数"},
    {"Unknown option: {}", "未知选项: {}"},

    {"Failed to initialize curl", "初始化 curl 失败"},
    {"Failed to open file for writing: {}", "无法打开文件写入: {}"},
    {"Download failed: {} ({})", "下载失败: {} ({})"},
    {"Download returned HTTP {} ({})", "下载返回 HTTP {} ({})"},
    {"Failed to fetch release info from GitHub API", "从 GitHub API 获取发布信息失败"},
    {"Release not found. Check the tag name.", "未找到该发布。请检查标签名称。"},
    {"Failed to parse GitHub API response: {}", "解析 GitHub API 响应失败: {}"},
    {"curl request failed: {} ({})", "curl 请求失败: {} ({})"},

    {"Downloading icon from GitHub...", "正在从 GitHub 下载图标..."},
    {"Failed to download icon file", "下载图标文件失败"},
    {"Icon downloaded to {}", "图标已下载到 {}"},
    {"Converting ICO to PNG...", "正在转换 ICO 为 PNG..."},
    {"Failed to convert ICO to PNG. Please install ImageMagick.",
     "ICO 转 PNG 失败。请安装 ImageMagick。"},
    {"Creating iconset at {}", "正在创建图标集于 {}"},
    {"Failed to resize icon to {}x{}", "调整图标大小为 {}x{} 失败"},
    {"Generating ICNS with iconutil...", "正在使用 iconutil 生成 ICNS..."},
    {"Failed to generate ICNS file", "生成 ICNS 文件失败"},
    {"ICNS generated at {}", "ICNS 已生成于 {}"},

    {"Failed to write launcher script to {}", "写入启动脚本失败至 {}"},

    {"Creating .app bundle at {}", "正在创建应用捆绑包于 {}"},
    {"Failed to write Info.plist to {}", "写入 Info.plist 失败至 {}"},
    {"Copied JAR to {}", "JAR 已复制到 {}"},
    {"Failed to copy JAR: {}", "复制 JAR 失败: {}"},
    {"Copied ICNS to {}", "ICNS 已复制到 {}"},
    {"Failed to copy ICNS: {}", "复制 ICNS 失败: {}"},
    {"ICNS not found, skipping icon", "未找到 ICNS，跳过图标"},
    {"Copied launcher to {}", "启动脚本已复制到 {}"},
    {"Failed to copy launcher: {}", "复制启动脚本失败: {}"},
    {"App bundle created at {}", "应用捆绑包已创建于 {}"},

    {"create-dmg not found. Install with: brew install create-dmg",
     "未找到 create-dmg。请通过 brew install create-dmg 安装"},
    {"Skipping DMG creation.", "跳过 DMG 创建。"},
    {"App bundle not found at {}", "应用捆绑包未找到于 {}"},
    {"Running: create-dmg for {}", "正在运行: 为 {} 创建 DMG"},
    {"create-dmg failed with exit code {}", "create-dmg 失败，退出码 {}"},
    {"DMG created at {}", "DMG 已创建于 {}"},

    {"Detecting local Java...", "正在检测本机 Java..."},
    {"Found Java {} at {}", "找到 Java {} 于 {}"},
    {"Failed to find Java 17+. Please install JDK 17 or later, or use --java-path to specify a path.",
     "未找到 Java 17+，请安装 JDK 17 或以上版本，或使用 --java-path 指定路径。"},
    {"Bundling Java runtime into .app...", "正在打包 Java 运行时到 .app..."},
    {"Java runtime bundled successfully", "Java 运行时打包成功"},
    {"Failed to bundle Java runtime", "Java 运行时打包失败"},
    {"--java-path requires a path argument", "--java-path 需要路径参数"},
};

std::string I18n::detectLang() {
    const char* env = std::getenv("LANG");
    if (env) {
        std::string lang(env);
        if (lang.size() >= 2 && lang.substr(0, 2) == "zh") {
            return "zh";
        }
    }
    return "en";
}

I18n& I18n::instance() {
    static I18n inst;
    return inst;
}

void I18n::setLang(const std::string& lang) {
    currentLang_ = lang;
    strings_.clear();
    if (lang == "zh") {
        strings_ = zh_strings;
        strings_["usage_text"] = zh_usage;
    } else {
        strings_["usage_text"] = en_usage;
    }
}

std::string I18n::t(const std::string& key) const {
    auto it = strings_.find(key);
    if (it != strings_.end()) {
        return it->second;
    }
    return key;
}

std::string I18n::format(const std::string& fmt, const std::vector<std::string>& args) const {
    std::string result;
    size_t pos = 0;
    size_t argIndex = 0;
    while (pos < fmt.size()) {
        size_t bracePos = fmt.find("{}", pos);
        if (bracePos == std::string::npos) {
            result += fmt.substr(pos);
            break;
        }
        result += fmt.substr(pos, bracePos - pos);
        if (argIndex < args.size()) {
            result += args[argIndex++];
        } else {
            result += "{}";
        }
        pos = bracePos + 2;
    }
    return result;
}
