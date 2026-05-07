cmake_minimum_required(VERSION 3.21 FATAL_ERROR)

project(compiler LANGUAGES CXX)
add_compile_definitions(TESTCASE_PATH="${PROJECT_SOURCE_DIR}/testcases")

set(CMAKE_CXX_STANDARD 17)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Configure CCache if available
find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
        set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
        set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

include_directories(third-party/magic_enum/include)
include_directories(SysYLexer SysYParser)
include_directories(third-party/antlr4-runtime)

file(GLOB_RECURSE SRC CONFIGURE_DEPENDS src/*.cpp)
file(GLOB_RECURSE INCLUDE CONFIGURE_DEPENDS include/*.hpp include/*.h)
file(GLOB_RECURSE ANTLR4_RUNTIME third-party/antlr4-runtime/*.cpp)
file(GLOB_RECURSE SysYLexer SysYLexer/*.cpp)
file(GLOB_RECURSE SysYParser SysYParser/*.cpp)

add_executable(${PROJECT_NAME} ${SRC} ${INCLUDE} ${ANTLR4_RUNTIME} ${SysYLexer} ${SysYParser}
        include/HIR-opt/SccpPass.h
        src/HIR-opt/SccpPass.cpp
        src/HIR-opt/DePhiPass.cpp
        include/HIR-opt/Loop.h
        include/HIR-opt/LoopPass.h
        include/HIR-opt/AlgebraOptPass.h
        include/HIR-opt/GVNPass.h
        include/HIR-opt/GCMPass.h
        src/HIR-opt/InliningPass.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC include)
target_link_libraries(${PROJECT_NAME} Threads::Threads)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pg")