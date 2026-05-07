cmake_minimum_required(VERSION 3.21)

# 设置源文件
set(SOURCE_FILES parser.cpp)

# 添加 parser 静态库
add_library(parser STATIC ${SOURCE_FILES})

# 添加头文件路径
target_include_directories(parser PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# 链接依赖
target_link_libraries(parser PUBLIC ast)