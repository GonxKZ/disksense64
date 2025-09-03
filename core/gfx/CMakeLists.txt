add_library(core_gfx)

target_sources(core_gfx PRIVATE
    treemap.cpp
)

target_include_directories(core_gfx PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(core_gfx PRIVATE
    core_model
)
