cmake_minimum_required(VERSION 3.21)

# 设置源文件
set(SOURCE_FILES ast.cpp)

# 添加 ast 静态库
add_library(ast STATIC ${SOURCE_FILES})

# 添加头文件路径
target_include_directories(ast PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})