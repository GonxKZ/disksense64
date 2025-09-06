# Security audit and permission analysis library
add_library(lib_security
    security.c
    audit.c
    permissions.c
    vulnerabilities.c
    policies.c
)

# Set language for .c files explicitly to C
set_source_files_properties(
    security.c
    audit.c
    permissions.c
    vulnerabilities.c
    policies.c
    PROPERTIES LANGUAGE C
)

target_include_directories(lib_security PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(lib_security 
    PRIVATE lib_utils
    PRIVATE lib_metadata
)

# Define library alias
add_library(security::security ALIAS lib_security)