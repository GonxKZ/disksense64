add_executable(DiskSense.Gui WIN32 main.cpp)

target_link_libraries(DiskSense.Gui PRIVATE
    # Add core libraries here as they are created
)
target_link_options(DiskSense.Gui PRIVATE -static)
