# Create a library for UI components
add_library(gui_ui
    ui/mainwindow.h
    ui/mainwindow.cpp
)

target_include_directories(gui_ui PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
)

target_link_libraries(gui_ui PRIVATE
    Qt6::Widgets
    core_model
    core_scan
    core_index
    core_gfx
    lib_utils
)

# Create a library for custom components
add_library(gui_components
    components/treemapwidget.h
    components/treemapwidget.cpp
    components/resultsdisplay.h
    components/resultsdisplay.cpp
)

target_include_directories(gui_components PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/components
)

target_link_libraries(gui_components PRIVATE
    Qt6::Widgets
    core_model
    core_gfx
    lib_utils
)

# Main executable
add_executable(DiskSense.Gui 
    main.cpp
    qtreemap.cpp
)

target_link_libraries(DiskSense.Gui PRIVATE
    Qt6::Widgets
    gui_ui
    gui_components
    core_model
    core_scan
    core_index
    core_gfx
    lib_utils
)