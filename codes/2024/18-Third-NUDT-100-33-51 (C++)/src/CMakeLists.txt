# Now add subdirectories, ensuring src or any directory depending on ANTLR generated files is added after
add_subdirectory(backend)
add_subdirectory(frontend)
add_subdirectory(midend)
add_subdirectory(utils)

# Define the main library
add_library(sysy STATIC ${ALL_OBJECT_FILES})
target_include_directories(sysy PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/frontend)
target_link_libraries(sysy PRIVATE SysYParser)

# Libraries and threading setup
set(SYSY_LIBS
        sysy_backend
        sysy_frontend
        sysy_midend
        sysy_utils
        )

set(THREADS_PREFER_PTHREAD_FLAG ON)
SET(CMAKE_BUILD_TYPE DEBUG)
find_package(Threads REQUIRED)

set(SYSY_THIRDPARTY_LIBS
        )

target_link_libraries(sysy PRIVATE Threads::Threads
        ${SYSY_LIBS}
        ${SYSY_THIRDPARTY_LIBS})

target_include_directories(
  sysy PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)