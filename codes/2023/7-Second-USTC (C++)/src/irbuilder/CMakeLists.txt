add_library(
    irbuilder
    sysY_irbuilder.cc
)

target_link_libraries(
    irbuilder
    ast
    mir
    utils
)