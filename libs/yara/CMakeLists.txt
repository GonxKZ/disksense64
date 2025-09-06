# YARA malware detection library
add_library(lib_yara
    yara.c
    yara.h
    rules.c
    rules.h
    scanner.c
    scanner.h
)

target_include_directories(lib_yara PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(lib_yara 
    PRIVATE lib_utils
)

# Define library alias
add_library(yara::yara ALIAS lib_yara)