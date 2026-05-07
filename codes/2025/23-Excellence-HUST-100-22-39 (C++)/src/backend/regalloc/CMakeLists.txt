cmake_minimum_required(VERSION 3.21)

# 设置源文件
set(SOURCE_FILES register.cpp reg_alloc.cpp)

# 创建 regalloc 静态库
add_library(regalloc STATIC ${SOURCE_FILES})

# 设置头文件路径
target_include_directories(regalloc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# 链接依赖
target_link_libraries(regalloc PUBLIC codegen)