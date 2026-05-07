enable_testing()

add_custom_target(testcase_test
    COMMAND ${CMAKE_CTEST_COMMAND} --verbose --timeout 150000
    DEPENDS compiler
    USES_TERMINAL
)

add_test(NAME bi_sheng_case
    COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_driver.sh ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests
)

# add_custom_target(starry_run_tests
#     COMMAND ${CMAKE_CTEST_COMMAND} --force-new-ctest-process # --verbose
#     --output-on-failure --progress
#     DEPENDS CloningTestcases
#     USES_TERMINAL
# )

# cmake_policy(PUSH)
# if(POLICY CMP0037)
#   cmake_policy(SET CMP0037 OLD)
#   add_custom_target(test DEPENDS starry_run_tests)
# endif()
# cmake_policy(POP)

# # add_test(NAME <name> [CONFIGURATIONS [Debug|Release|...]]
# #            [WORKING_DIRECTORY dir]
# #            COMMAND <command> [arg1 [arg2 ...]])

# set(TEST_CASES_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/tests/testcases)

# if(NOT EXISTS ${TEST_CASES_FOLDER})
#     add_custom_target(CloningTestcases
#         COMMAND ${CMAKE_COMMAND} -E echo "Cloning testcases..."
#         COMMAND git clone --verbose https://gitlab.eduxiji.net/csc1/nscscc/compiler2023.git
#         COMMAND ${CMAKE_COMMAND} -E make_directory ${TEST_CASES_FOLDER}
#         COMMAND ${CMAKE_COMMAND} -E rename
#             "${CMAKE_BINARY_DIR}/compiler2023/公开样例与运行时库"
#             "${TEST_CASES_FOLDER}/"
#         COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_SOURCE_DIR}/compiler2023
#         COMMAND ${CMAKE_COMMAND} -E echo "complete"
#         WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
#         COMMENT "Cloning and setting up testcases"
#         USES_TERMINAL
#     )
# else()
#     add_custom_target(CloningTestcases)
# endif()

# function(add_test_target NAME)
# #   string(REPLACE "/" "-" NAME ${NAME})
#   add_custom_target("test_${NAME}" COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -R "^${NAME}$$"
# )

# endfunction()

# add_test(NAME testcase
#            COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_driver.sh ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}
#            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests
#            )
# add_test_target("testcase")
