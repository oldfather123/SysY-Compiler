cmake_minimum_required(VERSION 3.10)
project(compiler)

file(GLOB SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(compiler ${SRC})