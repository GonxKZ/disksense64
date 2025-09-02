cmake_minimum_required(VERSION 3.20)

# Options
option(BUILD_CLI_ONLY "Build only the CLI application" OFF)
option(ENABLE_GUI "Enable GUI components" ON)

# Platform detection
if(WIN32)
    set(PLATFORM_WINDOWS ON)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(PLATFORM_WINDOWS_X64 ON)
    else()
        set(PLATFORM_WINDOWS_X86 ON)
    endif()
else()
    set(PLATFORM_WINDOWS OFF)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(PLATFORM_LINUX_X64 ON)
    else()
        set(PLATFORM_LINUX_X86 ON)
    endif()
endif()

if(CMAKE_TOOLCHAIN_FILE)
    include(${CMAKE_TOOLCHAIN_FILE})
endif()

project(DiskSense64 LANGUAGES C CXX)

# Include warnings and security flags
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/warnings.cmake)

# Platform-specific settings
if(PLATFORM_WINDOWS)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--stack,8388608")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# --- Include directories ---
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/core/engine
    ${CMAKE_CURRENT_SOURCE_DIR}/core/scan
    ${CMAKE_CURRENT_SOURCE_DIR}/core/usn
    ${CMAKE_CURRENT_SOURCE_DIR}/core/index
    ${CMAKE_CURRENT_SOURCE_DIR}/core/model
    ${CMAKE_CURRENT_SOURCE_DIR}/core/ops
    ${CMAKE_CURRENT_SOURCE_DIR}/core/gfx
    ${CMAKE_CURRENT_SOURCE_DIR}/core/regcom
    ${CMAKE_CURRENT_SOURCE_DIR}/core/vss
    ${CMAKE_CURRENT_SOURCE_DIR}/core/rules
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/chash
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/phash
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/audfp
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/peparse
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/lsh
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/fswin
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/util
)

# --- Applications ---
if(ENABLE_GUI AND NOT BUILD_CLI_ONLY)
    add_subdirectory(apps/DiskSense.Gui)
endif()
add_subdirectory(apps/DiskSense.Cli)

# --- Core Libraries ---
add_subdirectory(core/engine)
add_subdirectory(core/scan)
add_subdirectory(core/usn)
add_subdirectory(core/index)
add_subdirectory(core/model)
add_subdirectory(core/ops)
if(ENABLE_GUI AND NOT BUILD_CLI_ONLY)
    add_subdirectory(core/gfx)
    add_subdirectory(core/regcom)
    add_subdirectory(core/vss)
    add_subdirectory(core/rules)
endif()

# --- C/C++ Libraries ---
add_subdirectory(libs/chash)
add_subdirectory(libs/phash)
add_subdirectory(libs/utils)
if(ENABLE_GUI AND NOT BUILD_CLI_ONLY)
    add_subdirectory(libs/audfp)
    add_subdirectory(libs/peparse)
    add_subdirectory(libs/lsh)
endif()

# --- Platform Wrappers ---
add_subdirectory(platform/fswin)
add_subdirectory(platform/util)

# --- Tests ---
enable_testing()
add_subdirectory(tests)
