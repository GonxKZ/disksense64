add_library(platform_fswin)

target_sources(platform_fswin PRIVATE
    fswin.cpp
)

target_include_directories(platform_fswin PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
