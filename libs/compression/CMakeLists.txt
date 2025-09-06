

# Compressed data analysis library
add_library(lib_compression
    compression.c
)

target_include_directories(lib_compression PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)



# Define library alias
add_library(compression::compression ALIAS lib_compression)

target_link_libraries(lib_compression
    PRIVATE lib_utils LibArchive::LibArchive
    PUBLIC bsd
)
