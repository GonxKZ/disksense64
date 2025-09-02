add_library(core_scan
    scan.cpp
    scanner.cpp
)

target_include_directories(core_scan PRIVATE ../../)
target_link_libraries(core_scan PRIVATE lib_chash)

