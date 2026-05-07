cmake_minimum_required(VERSION 3.21)

# 添加子目录
add_subdirectory(ir)    # 中间代码生成
add_subdirectory(opt)   # 中间代码优化

# 创建 middleend 接口库
add_library(middleend INTERFACE)

# 链接依赖
target_link_libraries(middleend INTERFACE opt ir)