# Hidden files and rootkits detection library
add_library(lib_hidden
    hidden.c
    hidden.h
    rootkits.c
    rootkits.h
)

target_include_directories(lib_hidden PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(lib_hidden 
    PRIVATE lib_utils
    PRIVATE lib_metadata
)

# Define library alias
add_library(hidden::hidden ALIAS lib_hidden)