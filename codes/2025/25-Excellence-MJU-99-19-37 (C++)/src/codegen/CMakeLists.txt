# 允许继续使用 codegen_lib 名称（抑制警告）
# cmake_policy(SET CMP0171 OLD)


add_library(
    codegen_lib #STATIC
    CodeGen.cpp
    Register.cpp
)

target_link_libraries(codegen_lib common IR_lib)
