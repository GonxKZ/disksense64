# File carving library
add_library(lib_carving
    carving.c
    carving.h
    signatures.c
    signatures.h
)

target_include_directories(lib_carving PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(lib_carving 
    PRIVATE lib_utils
)

# Define library alias
add_library(carving::carving ALIAS lib_carving)