cmake_minimum_required(VERSION 3.0)

# 项目名称
project(compiler)

# note: PROJECT_SOURCE_DIR 是当前 CMakeLists.txt 文件所在的目录
set(HOME ${PROJECT_SOURCE_DIR}/workspace)
set(MAIN ${HOME}/source/compiler.cpp)

# 检查路径，打印HOME和TEST路径
message(STATUS "HOME: ${HOME}")

# 递归搜索包含目录
file(GLOB_RECURSE HEADER_FILES "${HOME}/include/*.h" "${HOME}/include/*.hpp")
set(INCLUDE_DIRS "")
foreach(header ${HEADER_FILES})
    get_filename_component(dir ${header} DIRECTORY)
    list(APPEND INCLUDE_DIRS ${dir})
endforeach()
list(REMOVE_DUPLICATES INCLUDE_DIRS)
message(STATUS "Include directories: ${INCLUDE_DIRS}")

# 使用 file(GLOB_RECURSE) 自动递归搜集 ${HOME}/source 目录中的源文件
file(GLOB_RECURSE SOURCE_FILES "${HOME}/source/**/*.cpp")

# 过滤掉 source/controller 文件夹下的源文件
foreach(file ${SOURCE_FILES})
    # if(file MATCHES "${HOME}/source/backend/controller/" OR file MATCHES ".*old\\.cpp$")
    if(file MATCHES ".*old\\.cpp$")
        list(REMOVE_ITEM SOURCE_FILES ${file})
    endif()
endforeach()

# 将所有源文件合并到一起
set(SOURCES
    ${SOURCE_FILES}
    ${MAIN}
)

include_directories(${INCLUDE_DIRS})

add_executable(compiler ${SOURCES})

target_compile_options(compiler PUBLIC -Wall)

set(CMAKE_CXX_FLAGS_DEBUG "-g -DLOCAL")
