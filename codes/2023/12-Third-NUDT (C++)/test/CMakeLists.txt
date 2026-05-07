cmake_minimum_required(VERSION 3.16)

project(test)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror ")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror ")

file(GLOB SY_TEST_FILES "sy/*.sy")
file(COPY ${SY_TEST_FILES} DESTINATION "sy")
file(GLOB C_TEST_FILE "c/*.c")
file(COPY ${C_TEST_FILE} DESTINATION "c")

aux_source_directory(. SRC_FILES)

add_executable(test ${SRC_FILES})

target_link_libraries(
  test
  PUBLIC
  asm
  visitor
  ir
  global
  util
  antlr4  
  ${3RD_PARTY_LIBS}
)
