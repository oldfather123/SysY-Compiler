# src/midend/CMakeLists.txt
add_library(midend_lib STATIC
    IR.cpp
    SysYIRGenerator.cpp
    SysYIRPrinter.cpp
    Pass/Pass.cpp
    Pass/Analysis/Dom.cpp
    Pass/Analysis/Liveness.cpp
    Pass/Analysis/Loop.cpp
    Pass/Analysis/LoopCharacteristics.cpp
    Pass/Analysis/LoopVectorization.cpp
    Pass/Analysis/AliasAnalysis.cpp
    Pass/Analysis/SideEffectAnalysis.cpp
    Pass/Analysis/CallGraphAnalysis.cpp
    Pass/Optimize/DCE.cpp
    Pass/Optimize/Mem2Reg.cpp
    Pass/Optimize/Reg2Mem.cpp
    Pass/Optimize/GVN.cpp
    Pass/Optimize/SysYIRCFGOpt.cpp
    Pass/Optimize/SCCP.cpp
    Pass/Optimize/LoopNormalization.cpp
    Pass/Optimize/LICM.cpp
    Pass/Optimize/LoopStrengthReduction.cpp
    Pass/Optimize/InductionVariableElimination.cpp
    Pass/Optimize/GlobalStrengthReduction.cpp
    Pass/Optimize/BuildCFG.cpp
    Pass/Optimize/TailCallOpt.cpp
)

# 包含中端模块所需的头文件路径
target_include_directories(midend_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/midend            # 中端顶层头文件
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/midend/Pass       # 增加 Pass 头文件路径
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/midend/Pass/Analysis # 增加 Pass/Analysis 头文件路径
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/midend/Pass/Optimize # 增加 Pass/Optimize 头文件路径
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/frontend          # 增加 frontend 头文件路径 (已存在)
    ${ANTLR_RUNTIME}/runtime/src # ANTLR运行时库头文件
)