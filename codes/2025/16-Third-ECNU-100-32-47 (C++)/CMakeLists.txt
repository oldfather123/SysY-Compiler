cmake_minimum_required(VERSION 3.10)

project(AAAC LANGUAGES CXX VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-g -Wno-format -Wno-attributes -Wall)
# add_compile_options(-O3 )
enable_testing()

add_subdirectory(src)
# add_subdirectory(src/frontend)
# add_subdirectory(tests)
file(GLOB SY_TEST_FILES "tests/functional/*.sy")
foreach(SY_TEST_FILE ${SY_TEST_FILES})
    get_filename_component(TEST_NAME ${SY_TEST_FILE} NAME)
    get_filename_component(TEST_NAME_WE ${TEST_NAME} NAME_WE)
    add_test(
        NAME functional_${TEST_NAME_WE}
        COMMAND sh -c "timeout 180 $<TARGET_FILE:main> < ${SY_TEST_FILE}"
    )
endforeach()

set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/output)