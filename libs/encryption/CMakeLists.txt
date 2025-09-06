# Encrypted data analysis library
add_library(lib_encryption
    encryption.c
    encryption.h
    detection.c
    detection.h
    ciphers.c
    ciphers.h
    entropy.c
    entropy.h
)

target_include_directories(lib_encryption PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(lib_encryption 
    PRIVATE lib_utils
)

# Define library alias
add_library(encryption::encryption ALIAS lib_encryption)