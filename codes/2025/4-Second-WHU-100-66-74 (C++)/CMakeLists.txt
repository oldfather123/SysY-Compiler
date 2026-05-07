cmake_minimum_required(VERSION 3.28)
project(CompilerRiscv)

set(CMAKE_CXX_STANDARD 20)

include_directories(${CMAKE_SOURCE_DIR}/src)

add_executable(CompilerRiscv src/main/main.cpp src/frontend/type.cpp)
