add_library(core_gfx)

target_sources(core_gfx PRIVATE
    treemap.cpp
    charts.cpp
    3dtreemap.cpp
)

target_include_directories(core_gfx PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(core_gfx PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    core_model
)
