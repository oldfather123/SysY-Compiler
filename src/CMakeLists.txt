cmake_minimum_required(VERSION 3.16)
project(SysY LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG" CACHE STRING "Release flags" FORCE)

find_package(Threads REQUIRED)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "*.cpp"
)

list(FILTER SOURCES EXCLUDE REGEX "^${CMAKE_CURRENT_BINARY_DIR}/.*")

add_executable(compiler ${SOURCES})

target_include_directories(compiler PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/frontend/generated"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/antlr4-runtime"
)

target_link_libraries(compiler PRIVATE Threads::Threads)
