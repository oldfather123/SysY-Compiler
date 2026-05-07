add_library(
    passes STATIC
    DeadCode.cpp
    Dominators.cpp
    LICM.cpp
    LoopDetection.cpp
    PureFuncDect.cpp
    Mem2Reg.cpp
)