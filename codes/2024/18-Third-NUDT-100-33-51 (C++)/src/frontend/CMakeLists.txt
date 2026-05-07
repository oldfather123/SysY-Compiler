# Generate lexer and parser with ANTLR
list(APPEND CMAKE_MODULE_PATH "${ANTLR_RUNTIME}/cmake")
include(FindANTLR)
antlr_target(SysYGen SysY.g4
  LEXER PARSER
  OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  VISITOR
)

# Add the ANTLR4 parser library
add_library(SysYParser SHARED ${ANTLR_SysYGen_CXX_OUTPUTS})
target_include_directories(SysYParser PUBLIC ${ANTLR_RUNTIME}/runtime/src)
target_link_libraries(SysYParser PUBLIC antlr4_shared)

add_library(
  sysy_frontend
  OBJECT
  IR.cpp
  SysYIRGenerator.cpp
  SysYPrinter.cpp)

target_include_directories(sysy_frontend PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(sysy_frontend PRIVATE SysYParser)

set(ALL_OBJECT_FILES
  ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:sysy_frontend>
  PARENT_SCOPE)
