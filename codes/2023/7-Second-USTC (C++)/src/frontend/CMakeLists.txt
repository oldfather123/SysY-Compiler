add_library(
        driver
        sysY_driver.cc
)

add_library(
        scanner STATIC
        sysY_scanner.cc
)

add_library(
        parser STATIC
        sysY_parser.cc
)

target_link_libraries(
        driver
        parser
        scanner
        ast
        utils
)