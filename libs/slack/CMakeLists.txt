# Slack space and unallocated space analysis library
add_library(lib_slack
    slack.c
    slack.h
    filesystem.c
    filesystem.h
)

target_include_directories(lib_slack PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(lib_slack 
    PRIVATE lib_utils
)

# Define library alias
add_library(slack::slack ALIAS lib_slack)