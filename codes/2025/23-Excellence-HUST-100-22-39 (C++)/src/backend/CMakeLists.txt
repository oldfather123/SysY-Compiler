cmake_minimum_required(VERSION 3.21)

# 添加子目录
add_subdirectory(codegen)   # 汇编代码生成
add_subdirectory(regalloc)  # 寄存器分配

# 创建 backend 接口库
add_library(backend INTERFACE)

# 链接依赖
target_link_libraries(backend INTERFACE codegen regalloc)