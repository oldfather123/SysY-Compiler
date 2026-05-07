add_library(
    codegen STATIC
    CodeGen.cpp
    Register.cpp
    LiveVariableAnalysis.cpp
)

target_link_libraries(codegen common IR_lib)
