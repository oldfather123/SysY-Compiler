cmake_minimum_required(VERSION 3.16)
project(anuc)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_COMPILER "g++")

file(GLOB_RECURSE SRC "src/*.cpp")
file(GLOB_RECURSE ANTLR "antlr/*.cpp")

include_directories(anuc third_party/antlr4-runtime src/ADT src/Analysis src/Backend src/IR
        src/PassDriver src/TransFormer src/Frontend antlr)
link_directories(third_party/lib)
add_executable(compiler  ${SRC} ${ANTLR})
target_link_libraries(compiler libantlr4-runtime.a)

#google test
#set(GTest_DIR /usr/local/lib/cmake/GTest)
#find_package(GTest REQUIRED)
#include_directories(${GTEST_INCLUDE_DIRS})
#enable_testing()

#测试ir
#add_executable(irtest _unittest/IR/test.cc ${LIB_SRC})
#target_link_libraries(irtest GTest::gtest GTest::gtest_main)
#add_test(AllTest irtest)

