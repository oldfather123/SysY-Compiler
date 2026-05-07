add_executable(
    compiler
    compiler.cpp
    SysYBuilder.cpp
)

target_link_libraries(
    compiler
    codegen
    common
    passes
    IR_lib
    syntax
    stdc++fs
)

install(
    TARGETS compiler
    RUNTIME DESTINATION bin
)
