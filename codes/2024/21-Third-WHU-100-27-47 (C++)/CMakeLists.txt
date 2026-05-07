cmake_minimum_required(VERSION 3.8)  # 指定 CMake 的最低版本要求
project(compiler)  # 定义项目名称

# 设置 C++ 编译器
SET (CMAKE_CXX_COMPILER "g++")

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 确保 RTTI 开启
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -frtti")

set(GENERATED_DIR "${CMAKE_SOURCE_DIR}/src/frontend/generated")

message(STATUS "Using generated directory")
include_directories("${GENERATED_DIR}")
# 源文件列表
file(GLOB SOURCES
"src/main.cpp"
"src/common/*.cpp"
"src/backend/*.cpp"
"src/frontend/*.cpp"
"src/frontend/generated/*.cpp"
"src/midend/ir/*.cpp"
"src/midend/utils/*.cpp"
"src/midend/transforms/*.cpp"
"src/midend/analyses/*.cpp"
)


# 包含目录
include_directories(src)
include_directories(src/common)
include_directories(src/frontend)
include_directories(src/backend)
include_directories(src/midend/ir)
include_directories(src/midend/utils)
include_directories(src/midend/transforms)
include_directories(src/midend/analyses)


# 编译选项
add_compile_options(-g -std=c++17 -O0 -lm) # -Wall
# add_compile_options(-std=c++17 -lm)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_definitions(-DDEBUG)
endif()

# 添加可执行文件
add_executable(compiler ${SOURCES})

# 在自己的机器上运行时，可以直接使用下面的代码链接动态库
# 设置库变量
set(LIBS antlr4-runtime)

# 链接库
target_link_libraries(compiler ${LIBS})

# 如果在测评机器上运行，需要使用下面的代码链接静态库
# # 设置静态库
# set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})

# # 使用 find_library 来查找库的路径
# find_library(ANTLR4_RUNTIME_LIBRARY
#              NAMES antlr4-runtime # 库的名字，不包括 'lib' 前缀和 '.a' 扩展名
#              PATHS "${CMAKE_SOURCE_DIR}/antlr4-runtime/lib" # 库可能的路径
#              NO_DEFAULT_PATH # 明确告诉 CMake 不要搜索默认的库路径
#              REQUIRED) # 表示这个库是必需的

# # 检查 find_library 是否成功找到了库
# if(ANTLR4_RUNTIME_LIBRARY)
#     # 如果找到了，使用 target_link_libraries 来链接库
#     target_link_libraries(compiler ${ANTLR4_RUNTIME_LIBRARY})
# else()
#     message(FATAL_ERROR "ANTLR4_RUNTIME_LIBRARY not found")
# endif()
