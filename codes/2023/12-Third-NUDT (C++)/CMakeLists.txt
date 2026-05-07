cmake_minimum_required(VERSION 3.16)

# In-source Builds Are Not Allowed
if(" ${CMAKE_SOURCE_DIR}" STREQUAL " ${CMAKE_BINARY_DIR}")
  message(
    FATAL_ERROR
      "FATAL: \nIn-source builds are not allowed.\nYou should create a separete directory for build files."
  )
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "RelWithDebInfo")
  message(
    STATUS "No CMAKE_BUILD_TYPE selected, defaulting to ${CMAKE_BUILD_TYPE}")
endif()

# Export Compile Commands
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_COMPILER "/usr/bin/clang")
set(CMAKE_CXX_COMPILER "/usr/bin/clang++")

# Add Src Files
file(GLOB_RECURSE ALL_SRC_FILES "${CMAKE_SOURCE_DIR}/src/*.[ch]"
     "${CMAKE_SOURCE_DIR}/src/*.[ch]pp" "${CMAKE_SOURCE_DIR}/test/*.[ch]"
     "${CMAKE_SOURCE_DIR}/test/*.[ch]pp")

# Add All Includes
include_directories(
  "${CMAKE_SOURCE_DIR}/3rd_party/antlr4/include"
  "${CMAKE_SOURCE_DIR}/3rd_party/googletest/include"
  "${CMAKE_SOURCE_DIR}/src/antlr4"
  "${CMAKE_SOURCE_DIR}/src/global"
  "${CMAKE_SOURCE_DIR}/src/ir"
  "${CMAKE_SOURCE_DIR}/src/util"
  "${CMAKE_SOURCE_DIR}/src/asm"
  "${CMAKE_SOURCE_DIR}/src/visitor"
  "${CMAKE_SOURCE_DIR}/src/pass"
  "${CMAKE_SOURCE_DIR}/src/error")

# Find Libs
link_directories("3rd_party/antlr4/lib" "3rd_party/googletest/lib")
set(3RD_PARTY_LIBS "libantlr4-runtime.a" "libgmock_main.a" "libgmock.a"
                   "libgtest_main.a" "libgtest.a")

# Add Src
add_subdirectory(src)
add_subdirectory(test)

# Format Check
project(formatting)
include(formatting.cmake)
clang_format(format ${ALL_SRC_FILES})
clang_format_check(format_check ${ALL_SRC_FILES})
