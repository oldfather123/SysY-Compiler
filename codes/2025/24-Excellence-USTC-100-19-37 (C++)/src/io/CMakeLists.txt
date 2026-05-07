add_library(sysy sylib.c)

install(
    TARGETS sysy
    ARCHIVE DESTINATION lib
)
