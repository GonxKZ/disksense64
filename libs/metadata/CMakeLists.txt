# Metadata analysis library
add_library(lib_metadata
    metadata.c
    metadata.h
)

target_include_directories(lib_metadata PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(lib_metadata 
    PRIVATE lib_utils
)

# Define library alias
add_library(metadata::metadata ALIAS lib_metadata)