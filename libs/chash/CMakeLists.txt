add_library(lib_chash
    sha256.c
    blake3.c
)

target_include_directories(lib_chash PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
