add_library(lib_utils)

target_sources(lib_utils PRIVATE
    utils.cpp
)

target_include_directories(lib_utils PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)