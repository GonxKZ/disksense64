# Create a library for UI components
add_library(gui_ui
    ui/mainwindow.h
    ui/mainwindow.cpp
    ui/dashboardtab.h
    ui/dashboardtab.cpp
    ui/settingsdialog.h
    ui/settingsdialog.cpp
    ui/onboardingdialog.h
    ui/onboardingdialog.cpp
    ui/tourwizard.h
    ui/tourwizard.cpp
    ui/trashrestoredialog.h
    ui/trashrestoredialog.cpp
    ui/fileexplorer.h
    ui/fileexplorer.cpp
    ui/advancedfilterwidget.h
    ui/advancedfilterwidget.cpp
    ui/exportdialog.h
    ui/exportdialog.cpp
    ui/comparisondialog.h
    ui/comparisondialog.cpp
)

target_include_directories(gui_ui PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(gui_ui PRIVATE
    Qt6::Widgets
    Qt6::Sql
    Qt6::Svg
    core_model
    core_scan
    core_index
    core_gfx
    core_cache
    core_database
    core_analysis
    core_ops
    platform_util
    lib_yara
    lib_utils
)

# Create a library for custom components
add_library(gui_components
    components/treemapwidget.h
    components/treemapwidget.cpp
    components/resultsdisplay.h
    components/resultsdisplay.cpp
    components/visualizationwidget.h
    components/visualizationwidget.cpp
    components/sunburstwidget.h
    components/sunburstwidget.cpp
)

target_include_directories(gui_components PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/components
)

target_link_libraries(gui_components PRIVATE
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    core_model
    core_gfx
    core_analysis
    lib_utils
)

# Main executable
add_executable(DiskSense.Gui 
    main.cpp
    qtreemap.cpp
)

target_link_libraries(DiskSense.Gui PRIVATE
    Qt6::Widgets
    Qt6::Sql
    Qt6::OpenGL
    gui_ui
    gui_components
    core_model
    core_scan
    core_index
    core_gfx
    core_cache
    core_database
    core_analysis
    lib_utils
)
