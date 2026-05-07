add_library(
  sysy_utils
  OBJECT
  SysyDebugger.cpp
  ConstantCounter.cpp)

set(ALL_OBJECT_FILES
  ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:sysy_utils>
  PARENT_SCOPE)
