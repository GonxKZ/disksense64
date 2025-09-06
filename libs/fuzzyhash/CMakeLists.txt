# Fuzzy hashing library (ssdeep, TLSH)
add_library(lib_fuzzyhash
    fuzzy_hash.c
    fuzzy_hash.h
    ssdeep.c
    ssdeep.h
    tlsh.c
    tlsh.h
)

target_include_directories(lib_fuzzyhash PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(lib_fuzzyhash 
    PRIVATE lib_utils
)

# Define library alias
add_library(fuzzyhash::fuzzyhash ALIAS lib_fuzzyhash)