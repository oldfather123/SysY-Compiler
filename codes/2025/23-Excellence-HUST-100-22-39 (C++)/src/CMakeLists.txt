cmake_minimum_required(VERSION 3.21)

# 添加子目录
add_subdirectory(frontend)  # 前端：词法分析、语法分析、语义分析
add_subdirectory(middleend) # 中端：中间代码生成、中间代码优化
add_subdirectory(backend)   # 后端：RISC-V 汇编代码生成

# 主程序：编译器入口
set(SOURCE_FILES main.cpp)

# 输出程序
add_executable(compiler ${SOURCE_FILES})

# 链接依赖
target_link_libraries(compiler PUBLIC frontend middleend backend)