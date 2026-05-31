# CMakeLists.txt — hmcl-mac-builder 构建配置
# 依赖：libcurl（必需）、nlohmann/json（自动下载）
cmake_minimum_required(VERSION 3.15)
project(hmcl-mac-builder VERSION 1.0.0 LANGUAGES CXX)

# 构建元信息，通过 compile_definitions 和 configure_file 注入代码
set(HMCL_MAC_BUILDER_REPO "https://github.com/SomeoneHX/hmcl-mac-builder")
set(HMCL_MAC_BUILDER_NAME "hmcl-mac-builder")

# 从模板生成 version.h（替换 @VAR@ 占位符）
configure_file(src/version.h.in ${CMAKE_CURRENT_BINARY_DIR}/version.h)

# C++17 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 系统依赖：libcurl
find_package(CURL REQUIRED)

# 自动下载 nlohmann/json 单头文件库
set(NLOHMANN_DIR ${CMAKE_CURRENT_BINARY_DIR}/nlohmann)
set(JSON_HEADER ${NLOHMANN_DIR}/json.hpp)
if(NOT EXISTS ${JSON_HEADER})
    file(MAKE_DIRECTORY ${NLOHMANN_DIR})
    file(DOWNLOAD https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp
         ${JSON_HEADER})
endif()

# 源文件列表（按功能模块分组）
set(SOURCES
    # 入口与配置
    src/main.cpp
    src/config.cpp
    # 国际化与工具
    src/i18n.cpp
    src/utils.cpp
    # 网络请求
    src/network.cpp
    # 图标处理
    src/icon.cpp
    # 启动脚本与 .app 打包
    src/launcher.cpp
    src/appbundle.cpp
    # Java 打包
    src/javabundle.cpp
    # DMG 生成
    src/dmg.cpp
)

add_executable(hmcl-mac-builder ${SOURCES})

# 链接 libcurl
target_link_libraries(hmcl-mac-builder
    PRIVATE
    CURL::libcurl
)

# 添加构建目录到包含路径（用于找到生成的 version.h）
target_include_directories(hmcl-mac-builder PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

# 注入仓库地址和工具名称作为编译时常量
target_compile_definitions(hmcl-mac-builder PRIVATE
    HMCL_MAC_BUILDER_REPO="${HMCL_MAC_BUILDER_REPO}"
    HMCL_MAC_BUILDER_NAME="${HMCL_MAC_BUILDER_NAME}"
)

# macOS 特定定义
if(APPLE)
    target_compile_definitions(hmcl-mac-builder PRIVATE __MACOS__)
endif()

# 安装到系统 bin 目录
install(TARGETS hmcl-mac-builder DESTINATION bin)
