add_library(common STATIC
    syntax_tree.c
    ast.cpp
    logging.cpp
)

set_source_files_properties(syntax_tree.c PROPERTIES LANGUAGE CXX)

target_link_libraries(common)
