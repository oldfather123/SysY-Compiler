cmake_minimum_required(VERSION 3.21)

# 设置源文件
set(SOURCE_FILES
    riscv.cpp
    riscv_value.cpp
    riscv_instruction.cpp
    riscv_basic_block.cpp
    riscv_function.cpp
    riscv_generator.cpp
    riscv_generator_select.cpp
    riscv_generator_emit.cpp
    riscv_optimize.cpp
    riscv_generator_print.cpp
)

# 创建 codegen 静态库
add_library(codegen STATIC ${SOURCE_FILES})

# 设置头文件路径
target_include_directories(codegen PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# 链接依赖
target_link_libraries(codegen PUBLIC ir regalloc)