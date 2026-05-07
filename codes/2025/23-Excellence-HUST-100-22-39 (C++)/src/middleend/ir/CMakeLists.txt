cmake_minimum_required(VERSION 3.21)

# 设置源文件
set(SOURCE_FILES
    ir.cpp
    ir_type.cpp
    ir_value.cpp
    ir_instruction.cpp
    ir_basic_block.cpp
    ir_function.cpp
    ir_module.cpp
    ir_builder.cpp
    ir_scope.cpp
    ir_generator.cpp
)

# 添加 ir 静态库
add_library(ir STATIC ${SOURCE_FILES})

# 添加头文件路径
target_include_directories(ir PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# 链接依赖
target_link_libraries(ir PUBLIC ast)