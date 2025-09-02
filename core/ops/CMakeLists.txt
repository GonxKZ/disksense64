add_library(core_ops)

target_sources(core_ops PRIVATE
    ops.cpp
    dedupe.cpp
)

target_include_directories(core_ops PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(core_ops PRIVATE
    core_model
    core_index
    lib_chash
)
