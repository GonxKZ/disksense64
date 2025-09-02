add_library(lib_phash)

target_sources(lib_phash PRIVATE
    phash.c
)

target_include_directories(lib_phash PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
