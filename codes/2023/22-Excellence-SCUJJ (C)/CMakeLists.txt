cmake_minimum_required(VERSION 3.0.0)
set(CMAKE_C_COMPILER "gcc")

project(compiler VERSION 2.1.0)

include(CTest)
enable_testing()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long -pedantic -O2")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W")

INCLUDE_DIRECTORIES(src/include/)
aux_source_directory(src/ DIR_SRCS)
aux_source_directory(src/std/ DIR_SRCS)
aux_source_directory(src/parser/ DIR_SRCS)
aux_source_directory(src/lexer/ DIR_SRCS)
aux_source_directory(src/util/ DIR_SRCS)
aux_source_directory(src/ir/ DIR_SRCS)
aux_source_directory(src/optimize/ DIR_SRCS)
aux_source_directory(src/gen/riscv/ DIR_SRCS)
aux_source_directory(src/include/ DIR_SRCS)

add_executable(compiler ${DIR_SRCS})
set(CMAKE_BUILD_TYPE Debug)
# set(CPACK_PROJECT_NAME ${PROJECT_NAME})
# set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
# include(CPack)
 