add_executable(
    sysy
    sysyc.cpp
    sysy_builder.cpp
)

target_link_libraries(
    sysy
    IR_lib
    common
    codegen_lib
    syntax
    stdc++fs
    passes
)

install(
    TARGETS sysy
    RUNTIME DESTINATION bin
)
