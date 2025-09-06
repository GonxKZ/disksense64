add_executable(DiskSense.Cli
    main.cpp
)

target_include_directories(DiskSense.Cli PRIVATE ../..)
# Link libraries in dependency order
target_link_libraries(DiskSense.Cli PRIVATE
    core_scan
    core_index
    core_ops
    # cleanup is part of core_ops
    lib_chash
    lib_phash
    lib_utils
    core_model
    platform_fswin
)

# Platform-specific linking
if(WIN32)
    target_link_libraries(DiskSense.Cli PRIVATE)
    target_link_options(DiskSense.Cli PRIVATE -static)
else()
    target_link_libraries(DiskSense.Cli PRIVATE pthread)
endif()
