# src/frontend/CMakeLists.txt
add_library(frontend_lib STATIC
    SysYBaseVisitor.cpp
    SysY.g4
    SysYLexer.cpp
    SysYParser.cpp
    SysYVisitor.cpp
)

# 包含前端模块所需的头文件路径
target_include_directories(frontend_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/frontend # 前端头文件
    ${ANTLR_RUNTIME}/runtime/src                    # ANTLR 运行时头文件
)

# 链接 ANTLR 运行时库
target_link_libraries(frontend_lib PRIVATE antlr4_shared)