cmake_minimum_required(VERSION 3.21)

# 添加子模块
add_subdirectory(ast)       # 语法树定义
add_subdirectory(lexer)     # 词法分析器
add_subdirectory(parser)    # 语法分析器

# 创建 frontend 接口库
add_library(frontend INTERFACE)

# 链接依赖
target_link_libraries(frontend INTERFACE lexer parser ast)