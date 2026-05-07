add_subdirectory(frontend)

SET(AAAC_LEX_PARSE_SOURCES
        ${PROJECT_SOURCE_DIR}/src/frontend/lexer/lex.cc
        ${PROJECT_SOURCE_DIR}/src/frontend/lexer/scannerbase.h
        ${PROJECT_SOURCE_DIR}/src/frontend/parser/parse.cc
        ${PROJECT_SOURCE_DIR}/src/frontend/parser/myparserbase.h
)

SET_SOURCE_FILES_PROPERTIES(${AAAC_LEX_PARSE_SOURCES} GENERATED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/frontend/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/frontend/parser)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/frontend/lexer)

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
        src_lex_parse_sources 
        DEPENDS $(AAAC_LEX_PARSE_SOURCES)
        COMMAND echo "lex parse souces generated from src" 
)

add_executable(main main.cc)
add_dependencies(main src_lex_parse_sources)

file(GLOB REG_ALLOCATE_SOURCES "reg_allocate/*.cc")
file(GLOB BACKEND_SOURCES "backend/*.cc")
file(GLOB LOWERING_SOURCES "lowering/*.cc")
file(GLOB MIDDLE_END_OPTIMIZATION_SOURCES "optimization/*.cc")
# file(GLOB FRONTEND_SOURCES "frontend/dump.cc" "frontend/AST/type.cc")
file(GLOB AAAC_FRONTEND_SOURCES
        "frontend/AST/*.cc"
        "frontend/parser/*.cc"
        "frontend/lexer/*.cc"
        "frontend/*.cc"
        "lowering/*.cc"
)

target_sources(main PRIVATE ${REG_ALLOCATE_SOURCES} ${BACKEND_SOURCES} ${LOWERING_SOURCES} ${MIDDLE_END_OPTIMIZATION_SOURCES} ${AAAC_FRONTEND_SOURCES} ${AAAC_LEX_PARSE_SOURCES})
target_link_libraries(main pthread)

install(TARGETS main)