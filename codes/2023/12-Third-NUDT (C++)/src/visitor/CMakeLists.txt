cmake_minimum_required(VERSION 3.16)

project(visitor)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror ")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror ")

aux_source_directory(. VISITOR_SRC_FILES)

add_library(visitor SHARED ${VISITOR_SRC_FILES})

target_link_directories(visitor PUBLIC ir antlr4)
