# Memory dumps analysis library
add_library(lib_memory
    memory.c
    memory.h
    dumps.c
    dumps.h
    processes.c
    processes.h
    network.c
    network.h
    analysis.c
    analysis.h
)

target_include_directories(lib_memory PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(lib_memory 
    PRIVATE lib_utils
)

# Define library alias
add_library(memory::memory ALIAS lib_memory)