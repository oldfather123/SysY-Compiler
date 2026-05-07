cmake_minimum_required(VERSION 3.16)

project(ir)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror ")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror ")

aux_source_directory(. IR_SRC_FILES)

add_library(ir SHARED ${IR_SRC_FILES})

target_link_directories(ir PUBLIC util)
