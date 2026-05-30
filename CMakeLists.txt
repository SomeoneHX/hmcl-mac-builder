cmake_minimum_required(VERSION 3.15)
project(hmcl-mac-builder VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(CURL REQUIRED)

set(NLOHMANN_DIR ${CMAKE_CURRENT_BINARY_DIR}/nlohmann)
set(JSON_HEADER ${NLOHMANN_DIR}/json.hpp)
if(NOT EXISTS ${JSON_HEADER})
    file(MAKE_DIRECTORY ${NLOHMANN_DIR})
    file(DOWNLOAD https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp
         ${JSON_HEADER})
endif()

set(SOURCES
    src/main.cpp
    src/config.cpp
    src/utils.cpp
    src/network.cpp
    src/icon.cpp
    src/launcher.cpp
    src/appbundle.cpp
    src/dmg.cpp
)

add_executable(hmcl-mac-builder ${SOURCES})

target_link_libraries(hmcl-mac-builder
    PRIVATE
    CURL::libcurl
)

target_include_directories(hmcl-mac-builder PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

if(APPLE)
    target_compile_definitions(hmcl-mac-builder PRIVATE __MACOS__)
endif()

install(TARGETS hmcl-mac-builder DESTINATION bin)
