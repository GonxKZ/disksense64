add_library(core_model)

target_sources(core_model PRIVATE
    model.cpp
)

target_include_directories(core_model PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
