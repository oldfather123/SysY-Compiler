add_library(
    codegen STATIC
    LivenessAnalysis.cpp
    InstructionSelector.cpp
    MachineBasicBlock.cpp
    MachineFunction.cpp
    MachineInstr.cpp
    MachineModule.cpp
    MIBuilder.cpp
    Operand.cpp
    RegisterAllocation.cpp
    PeepholeOptimization.cpp
    MIDCE.cpp
)

target_link_libraries(codegen common IR_lib)
