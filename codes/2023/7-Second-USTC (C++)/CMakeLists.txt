project(sysYCompiler)

cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 17)


# import LLVM
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in ${LLVM_DIR}")
llvm_map_components_to_libnames(
        llvm_libs
        support
        core
)

INCLUDE_DIRECTORIES(
    include
    include/frontend
    include/ast
    include/mir
    include/irbuilder
    include/optimize
    include/backend
    include/utils
    include/hasm
    ${LLVM_INCLUDE_DIRS}
)

add_definitions(${LLVM_DEFINITIONS})


add_subdirectory(src/frontend)
add_subdirectory(src/ast)
add_subdirectory(src/mir)
add_subdirectory(src/irbuilder)
add_subdirectory(src/optimize)
add_subdirectory(src/backend)
add_subdirectory(src/utils)
add_subdirectory(src/hasm)

add_executable(
        compiler
        src/main.cc
)

# target_compile_options(
#     compiler
#     PUBLIC
#     -g
#     -fsanitize=address
# )

target_link_libraries(
        compiler
        driver
        mir
        irbuilder
        optimize
        asbuilder
        utils
        hasm
        # asan
)

install(
        TARGETS compiler
        RUNTIME DESTINATION bin
)