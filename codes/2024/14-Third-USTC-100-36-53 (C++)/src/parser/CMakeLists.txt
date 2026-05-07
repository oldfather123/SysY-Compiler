flex_target(lex lexical_analyzer.l ${CMAKE_CURRENT_LIST_DIR}/lexical_analyzer.c)
bison_target(syntax syntax_analyzer.y
    ${CMAKE_CURRENT_LIST_DIR}/syntax_analyzer.c
    DEFINES_FILE ${CMAKE_CURRENT_LIST_DIR}/syntax_analyzer.h)

add_flex_bison_dependency(lex syntax)
add_library(syntax STATIC
    ${BISON_syntax_OUTPUTS}
    ${FLEX_lex_OUTPUTS}
)

set_source_files_properties(${BISON_syntax_OUTPUTS} PROPERTIES LANGUAGE CXX)
set_source_files_properties(${FLEX_lex_OUTPUTS} PROPERTIES LANGUAGE CXX)

include_directories(${CMAKE_CURRENT_LIST_DIR})
