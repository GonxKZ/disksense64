# System logs analysis library
add_library(lib_logs
    logs.c
    logs.h
    parsers.c
    parsers.h
    filters.c
    filters.h
)

target_include_directories(lib_logs PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(lib_logs 
    PRIVATE lib_utils
)

# Define library alias
add_library(logs::logs ALIAS lib_logs)