cmake_minimum_required(VERSION 3.16)
project(compile2025-0)

# 默认使用 Debug 模式（如果未指定）
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
endif ()

# 使用 C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

# 包含头文件目录
include_directories(SYSTEM include)

set(GENERATED_DIR "${CMAKE_SOURCE_DIR}/generated")
file(MAKE_DIRECTORY ${GENERATED_DIR})
include_directories(SYSTEM ${GENERATED_DIR})
include_directories(SYSTEM 3rd)

BISON_TARGET(
        Parser
        "${CMAKE_CURRENT_SOURCE_DIR}/src/sysy/parser.y"
        "${GENERATED_DIR}/parser.tab.cpp"
        DEFINES_FILE "${GENERATED_DIR}/parser.tab.hpp"
)

FLEX_TARGET(
        Lexer
        "${CMAKE_CURRENT_SOURCE_DIR}/src/sysy/lexer.l"
        "${GENERATED_DIR}/lexer.cpp"
)

ADD_FLEX_BISON_DEPENDENCY(Lexer Parser)


file(GLOB_RECURSE SOURCES
        src/ir/*.cpp
        src/parser/*.cpp
        src/reg/*.cpp
        src/riscv/*.cpp
        src/pass/*.cpp
        src/*.cpp
)

set(COMPILER_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
        ${SOURCES}
        ${BISON_Parser_OUTPUTS} ${FLEX_Lexer_OUTPUTS}
)


add_executable(compiler2025-0 ${COMPILER_SOURCES})
