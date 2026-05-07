cmake_minimum_required(VERSION 3.21)

# 设置源文件
set(SOURCE_FILES lexer.cpp)

# 添加 lexer 静态库
add_library(lexer STATIC ${SOURCE_FILES})

# 添加头文件路径
target_include_directories(lexer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# 链接依赖
target_link_libraries(lexer PUBLIC ast parser)