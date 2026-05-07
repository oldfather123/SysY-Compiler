include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lexer)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/parser)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)

add_compile_options(-g)

file(GLOB AAAC_FRONTEND_SOURCE
        "errmsg/*.cc"
        "AST/*.cc"
        ""
        "*.cc"
        "../lowering/*.cc"
)

SET(AAAC_LEX_PARSE_SOURCES
        ${PROJECT_SOURCE_DIR}/src/frontend/lexer/lex.cc
        ${PROJECT_SOURCE_DIR}/src/frontend/lexer/scannerbase.h
        ${PROJECT_SOURCE_DIR}/src/frontend/parser/myparse.cc
        ${PROJECT_SOURCE_DIR}/src/frontend/parser/myparserbase.h
)

SET_SOURCE_FILES_PROPERTIES(${AAAC_LEX_PARSE_SOURCES} GENERATED)

add_custom_command(
    OUTPUT ${PROJECT_SOURCE_DIR}/src/frontend/lexer/lex.cc ${PROJECT_SOURCE_DIR}/src/frontend/lexer/scannerbase.h
    COMMAND flexc++ aaac.lex --target-directory=${PROJECT_SOURCE_DIR}/src/frontend/lexer
    DEPENDS ${PROJECT_SOURCE_DIR}/src/frontend/lexer/aaac.lex
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/src/frontend/lexer
)

add_custom_command(
        OUTPUT ${PROJECT_SOURCE_DIR}/src/frontend/parser/parse.cc ${PROJECT_SOURCE_DIR}/src/frontend/parser/myparserbase.h
        # COMMAND ${CMAKE_COMMAND} -E touch ${PROJECT_SOURCE_DIR}/src/frontend/parser/Parserbase.h
        COMMAND bisonc++ aaac.y --target-directory=${PROJECT_SOURCE_DIR}/src/frontend/parser -f myparser
        DEPENDS ${PROJECT_SOURCE_DIR}/src/frontend/parser/aaac.y
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/src/frontend/parser
)

add_custom_target(
        lex_parse_sources 
        ALL
        DEPENDS $(AAAC_LEX_PARSE_SOURCES)
        COMMAND echo "lex parse sources generated"
)

# add_executable(test_lex ./tests/test_lex.cc ${AAAC_LEX_PARSE_SOURCES} ${AAAC_FRONTEND_SOURCE})
# add_dependencies(test_lex lex_parse_sources)


# add_executable(test_ast_print ./tests/test_ast_print.cc ${AAAC_LEX_PARSE_SOURCES} ${AAAC_FRONTEND_SOURCE})
# add_dependencies(test_ast_print lex_parse_sources)

# add_executable(test_syntax ./tests/test_syntax.cc ${AAAC_LEX_PARSE_SOURCES} ${AAAC_FRONTEND_SOURCE})
# add_dependencies(test_syntax lex_parse_sources)

# add_executable(test_sem ./tests/test_sem.cc ${AAAC_LEX_PARSE_SOURCES} ${AAAC_FRONTEND_SOURCE})
# add_dependencies(test_sem lex_parse_sources)

# add_executable(test_translate ./tests/test_translate.cc ${AAAC_LEX_PARSE_SOURCES} ${AAAC_FRONTEND_SOURCE})
# add_dependencies(test_translate lex_parse_sources)

# add_test(test_lex bash -c "./test_lex < ${CMAKE_CURRENT_SOURCE_DIR}/tests/input/test_syntax.sy")
# add_test(test_ast_print test_ast_print)
# add_test(test_syntax bash -c "./test_syntax < ${CMAKE_CURRENT_SOURCE_DIR}/tests/input/test_syntax.sy")
# add_test(test_sem bash -c "./test_sem < ${CMAKE_CURRENT_SOURCE_DIR}/tests/input/test_sem.sy")
# add_test(test_translate bash -c "./test_translate < ${CMAKE_CURRENT_SOURCE_DIR}/tests/input/test_translate.sy")
