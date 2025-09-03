# Test configuration for DiskSense64

# Unit tests
add_executable(test_unit
    unit/testmain.cpp
)

target_include_directories(test_unit PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/unit
    ${CMAKE_SOURCE_DIR}/apps/DiskSense.Gui
    ${CMAKE_SOURCE_DIR}/apps/DiskSense.Gui/ui
    ${CMAKE_SOURCE_DIR}/apps/DiskSense.Gui/components
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/core/model
    ${CMAKE_SOURCE_DIR}/core/scan
    ${CMAKE_SOURCE_DIR}/core/index
    ${CMAKE_SOURCE_DIR}/core/gfx
    ${CMAKE_SOURCE_DIR}/core/cache
    ${CMAKE_SOURCE_DIR}/core/database
    ${CMAKE_SOURCE_DIR}/core/analysis
    ${CMAKE_SOURCE_DIR}/core/security
    ${CMAKE_SOURCE_DIR}/core/network
    ${CMAKE_SOURCE_DIR}/core/automation
    ${CMAKE_SOURCE_DIR}/libs
)

target_link_libraries(test_unit PRIVATE
    Qt6::Test
    Qt6::Widgets
    Qt6::Sql
    Qt6::Network
    gui_ui
    gui_components
    core_model
    core_scan
    core_index
    core_gfx
    core_cache
    core_database
    core_analysis
    core_security
    core_network
    core_automation
    lib_utils
    lib_chash
    lib_phash
    lib_audfp
    lib_peparse
    lib_lsh
)

# Performance benchmarks
add_executable(bench_perf
    perf/benchmark.cpp
)

target_include_directories(bench_perf PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/perf
    ${CMAKE_SOURCE_DIR}/apps/DiskSense.Gui
    ${CMAKE_SOURCE_DIR}/apps/DiskSense.Gui/ui
    ${CMAKE_SOURCE_DIR}/apps/DiskSense.Gui/components
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/core/model
    ${CMAKE_SOURCE_DIR}/core/scan
    ${CMAKE_SOURCE_DIR}/core/index
    ${CMAKE_SOURCE_DIR}/core/gfx
    ${CMAKE_SOURCE_DIR}/core/cache
    ${CMAKE_SOURCE_DIR}/core/database
    ${CMAKE_SOURCE_DIR}/core/analysis
    ${CMAKE_SOURCE_DIR}/core/security
    ${CMAKE_SOURCE_DIR}/core/network
    ${CMAKE_SOURCE_DIR}/core/automation
    ${CMAKE_SOURCE_DIR}/libs
)

target_link_libraries(bench_perf PRIVATE
    Qt6::Test
    Qt6::Widgets
    Qt6::Sql
    Qt6::Network
    gui_ui
    gui_components
    core_model
    core_scan
    core_index
    core_gfx
    core_cache
    core_database
    core_analysis
    core_security
    core_network
    core_automation
    lib_utils
    lib_chash
    lib_phash
    lib_audfp
    lib_peparse
    lib_lsh
)

# Add tests to ctest
add_test(NAME unit_tests COMMAND test_unit)
add_test(NAME perf_benchmarks COMMAND bench_perf)

# Custom targets for running tests
add_custom_target(run_tests
    COMMAND test_unit
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running unit tests"
)

add_custom_target(run_benchmarks
    COMMAND bench_perf
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running performance benchmarks"
)

# Test coverage target (if coverage is enabled)
if(ENABLE_COVERAGE)
    add_custom_target(coverage
        COMMAND lcov --capture --directory . --output-file coverage.info
        COMMAND genhtml coverage.info --output-directory coverage_report
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating test coverage report"
    )
endif()