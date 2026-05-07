cmake_minimum_required (VERSION 3.19)

# cmake_policy(SET CMP0135 OLD)

set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(sysy
  VERSION 0.0.0.1
  DESCRIPTION "The SysY language compiler"
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ standard to conform to")
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  message(STATUS "Build type not set, falling back to Debug mode.")
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING
      "Choose the type of build, options are: Debug Release." FORCE)
endif(NOT CMAKE_BUILD_TYPE)

# Set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

# ANTLR
set(ANTLR_EXECUTABLE "${PROJECT_SOURCE_DIR}/antlr/antlr-4.13.2-complete.jar")
set(ANTLR_RUNTIME "${PROJECT_SOURCE_DIR}/antlr/antlr4-runtime-4.13.2")

set(ANTLR4_INSTALL ON)
set(WITH_DEMO OFF)
set(ANTLR_BUILD_CPP_TESTS OFF)
add_subdirectory(${ANTLR_RUNTIME})

# Project source files
add_subdirectory(src)