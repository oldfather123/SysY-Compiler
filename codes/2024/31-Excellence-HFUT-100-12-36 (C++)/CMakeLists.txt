cmake_minimum_required(VERSION 3.15)
project(compiler CXX)
file(GLOB_RECURSE DIR_SRC "src/*.cpp")
file(GLOB_RECURSE DIR_SRC_E "include/*.cpp")
set(CMAKE_EXE_LINKER_FLAGS "-static")
add_compile_options(-g -std=c++17 -O0)
# add_compile_options(-g -std=c++17
#     -fsanitize=address
#     -fno-omit-frame-pointer
# )
# add_link_options(-fsanitize=address)
# link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib)
include_directories(src)
# include_directories(src/antlr)
# include_directories(src/backend)
# include_directories(src/optimize)
# include_directories(src/ir)
# include_directories(include/antlr4-runtime)
# include_directories(include)
# include_directories(include/rapidjson)
add_executable(compiler ${DIR_SRC} ${DIR_SRC_E})
target_link_libraries(compiler pthread)

include(cmake/Tests.cmake)
