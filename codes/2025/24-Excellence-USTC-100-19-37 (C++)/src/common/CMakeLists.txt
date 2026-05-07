add_library(common STATIC
    ast.cpp
    syntax_tree.c
)

target_link_libraries(common)
