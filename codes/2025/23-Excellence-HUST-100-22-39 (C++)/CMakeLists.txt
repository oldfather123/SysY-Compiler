cmake_minimum_required(VERSION 3.21)

# 指定编译器：clang++
set(CMAKE_CXX_COMPILER clang++)

# 项目基本信息
project(ACompiler
        LANGUAGES CXX
        VERSION 1.0
        DESCRIPTION "AComplier: A Compiler for SysY2022 from 都别卷了让我来(T202510487205827)")

# 设置 C++ 标准：c++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 根据构建类型设置编译选项
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # Debug 模式：完整调试符号 + 关闭优化调试模式
    add_compile_options(-O0 -g3 -ggdb)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-O2)
endif()

# 添加链接选项
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lm")

# 设置输出目录
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR})

# 添加子目录
add_subdirectory(src)