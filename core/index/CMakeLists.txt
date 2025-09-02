add_library(core_index)

target_sources(core_index PRIVATE
    index.cpp
    lsm_index.cpp
)

target_include_directories(core_index PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(core_index PRIVATE
    core_model
)
