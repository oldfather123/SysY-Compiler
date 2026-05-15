cmake_minimum_required(VERSION 3.16)

if(NOT DEFINED CMAKE_CXX_COMPILER)
    set(CMAKE_CXX_COMPILER clang++-18 CACHE STRING "Default C++ compiler")
endif()

project(SysY LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

find_package(Threads REQUIRED)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "*.cpp"
)

list(FILTER SOURCES EXCLUDE REGEX "^${CMAKE_CURRENT_BINARY_DIR}/.*")

add_executable(compiler ${SOURCES})

target_compile_features(compiler PRIVATE cxx_std_17)

set_target_properties(compiler PROPERTIES
    CXX_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/.."
)

target_compile_options(compiler PRIVATE
    $<$<CONFIG:Release>:-O2 -DNDEBUG>
)

target_include_directories(compiler PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/frontend/generated"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/antlr4-runtime"
)

target_link_libraries(compiler PRIVATE Threads::Threads m)
