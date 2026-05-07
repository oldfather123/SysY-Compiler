# src/CMakeLists.txt
# add_subdirectory 命令会负责遍历子目录并查找其内部的 CMakeLists.txt 文件
add_subdirectory(frontend)
add_subdirectory(midend)
add_subdirectory(backend/RISCv64)

# 构建 sysyc 可执行文件，链接各个模块的库
add_executable(sysyc
    sysyc.cpp
)

# 链接各个模块的库
target_link_libraries(sysyc PRIVATE
    frontend_lib
    midend_lib
    riscv64_backend_lib
    antlr4_shared
)

# 设置 include 路径，包含项目顶层 include 目录
target_include_directories(sysyc PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include # 项目头文件目录
    ${ANTLR_RUNTIME}/runtime/src # ANTLR运行时库头文件
)