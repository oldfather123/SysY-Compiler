add_library(
  sysy_midend
  OBJECT
  ActiveVarAnalysis.cpp
  DataFlowAnalysis.cpp
  Sysyoptimization.cpp
  CFA.cpp
  Mem2Reg.cpp
  SCCP.cpp
  SysYLoopAnalysis.cpp
  DCE.cpp
  FuncAnalysis.cpp
  SysyLoopoptimization.cpp
  SysyLoopspecificate.cpp
  DFE.cpp
  Global2Local.cpp
  FuncInline.cpp
  SysYLoopPreStore.cpp
  SysYLoopUnroll.cpp
  TailCallElimination.cpp
  FunctionCache.cpp
  )

set(ALL_OBJECT_FILES
  ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:sysy_midend>
  PARENT_SCOPE)
