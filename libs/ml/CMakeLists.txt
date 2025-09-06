# Machine learning library
add_library(lib_ml
    ml.c
    ml_features.c
    classifiers.c
    training.c
    models.c
)

# Set language for .c files explicitly to C
set_source_files_properties(
    ml.c
    ml_features.c
    classifiers.c
    training.c
    models.c
    PROPERTIES LANGUAGE C
)

# Add compile definitions for GNU source
target_compile_definitions(lib_ml PRIVATE _GNU_SOURCE)

target_include_directories(lib_ml PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(lib_ml 
    PRIVATE lib_utils
    PRIVATE lib_fuzzyhash
    PRIVATE lib_metadata
)

# Define library alias
add_library(ml::ml ALIAS lib_ml)