cmake_minimum_required(VERSION 3.16)

project(pass)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror ")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror ")

aux_source_directory(. PASS_SRC_FILES)

add_library(pass SHARED ${PASS_SRC_FILES})
