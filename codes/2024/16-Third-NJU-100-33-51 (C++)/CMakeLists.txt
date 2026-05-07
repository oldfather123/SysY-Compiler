project(antpie)

find_library(ANTLR4_LIBRARY
  NAMES antlr4-runtime antlr-runtime
  PATHS /usr/local/lib
  REQUIRED)

message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

# if (CMAKE_BUILD_TYPE STREQUAL "Debug")
#   message("AddressSanitizer enabled")
#   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -g")
#   set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fsanitize=address")
# endif()

cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 17)


INCLUDE_DIRECTORIES(
    include/ir
    include/asm
    include/cgen
    include/opt
    include/util
    include/frontend
    include
    parser
    /usr/local/include/antlr4-runtime
)

add_subdirectory(src/ir)
add_subdirectory(src/asm)
add_subdirectory(src/cgen)
add_subdirectory(src/opt)
add_subdirectory(src/util)
add_subdirectory(src/frontend)
add_subdirectory(parser)

add_executable(
        compiler
        src/main.cc
)

target_link_libraries(
        compiler PRIVATE
        asm
        cgen
        opt
        frontend
        ir 
        opt
        util
        parser
        ${ANTLR4_LIBRARY}
)

install(
        TARGETS compiler
        RUNTIME DESTINATION bin
)