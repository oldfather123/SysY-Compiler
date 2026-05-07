# src/backend/RISCv64/CMakeLists.txt
add_library(riscv64_backend_lib STATIC
    RISCv64AsmPrinter.cpp
    RISCv64Backend.cpp
    RISCv64ISel.cpp
    RISCv64LLIR.cpp
    RISCv64RegAlloc.cpp
    RISCv64LinearScan.cpp
    RISCv64SimpleRegAlloc.cpp
    RISCv64BasicBlockAlloc.cpp
    Handler/CalleeSavedHandler.cpp
    Handler/LegalizeImmediates.cpp
    Handler/PrologueEpilogueInsertion.cpp
    Handler/EliminateFrameIndices.cpp
    Optimize/Peephole.cpp
    Optimize/PostRA_Scheduler.cpp
    Optimize/PreRA_Scheduler.cpp
    Optimize/DivStrengthReduction.cpp
)

# 包含后端模块所需的头文件路径
target_include_directories(riscv64_backend_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/backend/RISCv64 # 后端顶层头文件
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/backend/RISCv64/Handler # 增加 Handler 头文件路径
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/backend/RISCv64/Optimize # 增加 Optimize 头文件路径
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/midend         # 增加 midend 头文件路径 (已存在)
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/midend/Pass         # 增加 midend 头文件路径 (已存在)
)