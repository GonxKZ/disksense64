# File clustering library
add_library(lib_clustering
    clustering.c
    clustering.h
    algorithms.c
    algorithms.h
    distance.c
    distance.h
)

target_include_directories(lib_clustering PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(lib_clustering 
    PRIVATE lib_utils
    PRIVATE lib_fuzzyhash
)

# Define library alias
add_library(clustering::clustering ALIAS lib_clustering)