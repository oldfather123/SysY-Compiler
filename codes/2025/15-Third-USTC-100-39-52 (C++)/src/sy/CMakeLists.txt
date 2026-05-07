add_executable(
    compiler
    sy.cpp
    sy_builder.cpp
)

target_link_libraries(
    compiler
    IR_lib
    common
    syntax
    codegen
    stdc++fs
    opt_lib
)

install(
    TARGETS compiler
    RUNTIME DESTINATION bin
)