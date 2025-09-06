# Forensic image mounting library
add_library(lib_mount
    mount.c
    mount.h
    image_formats.c
    image_formats.h
)

target_include_directories(lib_mount PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(lib_mount 
    PRIVATE lib_utils
)

# Define library alias
add_library(mount::mount ALIAS lib_mount)