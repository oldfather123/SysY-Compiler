add_library(
  sysy_backend
  OBJECT
  Riscv.cpp
  SysyPhiDelete.cpp
  RiscvPrinter.cpp
  Mid2EndDAG.cpp
  BackEndActiveAnalysis.cpp
  DAGPrinter.cpp
  DAGBuilder.cpp
  RegisterAlloc.cpp
  MacroExpansion.cpp
  Mid2End.cpp
  DAG2End.cpp
  InstructionSchedule.cpp
  Peephole.cpp)

set(ALL_OBJECT_FILES
  ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:sysy_backend>
  PARENT_SCOPE)
