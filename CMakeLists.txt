cmake_minimum_required(VERSION 3.20)

# Project options
option(BUILD_CLI_ONLY "Build only the CLI application" OFF)
option(ENABLE_GUI "Enable GUI components" ON)
option(USE_QT_GUI "Use Qt for the GUI" ON)
option(ENABLE_TESTING "Enable testing" ON)
option(CROSS_PLATFORM_BUILD "Enable cross-platform build support" ON)
option(ENABLE_SANITIZERS "Enable Address/UB sanitizers (non-MSVC)" OFF)
option(ENABLE_WERROR "Treat warnings as errors" OFF)

# Platform detection
if(WIN32)
    set(PLATFORM_WINDOWS ON)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(PLATFORM_WINDOWS_X64 ON)
    else()
        set(PLATFORM_WINDOWS_X86 ON)
    endif()
    
    # Windows-specific settings
    add_definitions(-D_WIN32_WINNT=0x0A00) # Windows 10
    add_definitions(-DUNICODE -D_UNICODE)
    add_definitions(-DNOMINMAX)
else()
    set(PLATFORM_WINDOWS OFF)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(PLATFORM_LINUX_X64 ON)
    else()
        set(PLATFORM_LINUX_X86 ON)
    endif()
    
    # Linux-specific settings
    add_definitions(-DLINUX)
    add_definitions(-D_UNIX)
endif()

# Include toolchain file if specified
if(CMAKE_TOOLCHAIN_FILE)
    include(${CMAKE_TOOLCHAIN_FILE})
endif()

# Project definition
project(DiskSense64 LANGUAGES C CXX)
add_compile_definitions(_GNU_SOURCE)
add_compile_definitions(_DEFAULT_SOURCE)

# Add feature test flags for glibc compatibility


# Qt configuration
if(ENABLE_GUI AND USE_QT_GUI)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
    set(CMAKE_AUTOUIC ON)
    find_package(Qt6 COMPONENTS Widgets Test Sql Network OpenGL OpenGLWidgets Svg Concurrent REQUIRED)
endif()

# Find libarchive for compression/decompression
find_package(LibArchive REQUIRED)

# Include warnings and security flags
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/warnings.cmake)

# Platform-specific linker settings
if(PLATFORM_WINDOWS)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--stack,8388608")
    
    # Windows-specific libraries
    if(NOT BUILD_CLI_ONLY AND NOT USE_QT_GUI)
        set(WIN32_LIBS
            kernel32
            user32
            gdi32
            winspool
            shell32
            ole32
            oleaut32
            uuid
            comdlg32
            advapi32
        )
        
        # Graphics libraries for GUI
        if(ENABLE_GUI)
            list(APPEND WIN32_LIBS
                d2d1
                dwrite
                dcomp
                windowscodecs
            )
        endif()
        
        # Link Windows libraries
        link_libraries(${WIN32_LIBS})
    endif()
else()
    # Linux-specific libraries
    if(NOT BUILD_CLI_ONLY AND NOT USE_QT_GUI)
        set(LINUX_LIBS
            pthread
            dl
            rt
            m
        )
        
        # Graphics libraries for GUI
        if(ENABLE_GUI)
            list(APPEND LINUX_LIBS
                X11
                cairo
                pango
                pangocairo
                glib-2.0
                gdk_pixbuf-2.0
            )
        endif()
        
        # Link Linux libraries
        link_libraries(${LINUX_LIBS})
    endif()
endif()

# C++ standard settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Global developer options
if(ENABLE_WERROR)
  if(MSVC)
    add_compile_options(/W4 /WX)
  else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
  endif()
else()
  if(MSVC)
    add_compile_options(/W4)
  else()
    add_compile_options(-Wall -Wextra -Wpedantic)
  endif()
endif()

if(ENABLE_SANITIZERS AND NOT MSVC)
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address,undefined)
endif()

# Output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/apps/DiskSense.Gui
    ${CMAKE_CURRENT_SOURCE_DIR}/apps/DiskSense.Gui/ui
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

# Platform-specific include directories
if(PLATFORM_WINDOWS)
    include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/platform/win32
    )
else()
    include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/platform/linux
    )
endif()

# Core applications
if(ENABLE_GUI AND NOT BUILD_CLI_ONLY)
    add_subdirectory(apps/DiskSense.Gui)
endif()

add_subdirectory(apps/DiskSense.Cli)

# Core libraries
add_subdirectory(core/engine)
add_subdirectory(core/scan)
add_subdirectory(core/usn)
add_subdirectory(core/index)
add_subdirectory(core/model)
add_subdirectory(core/ops)
if(ENABLE_GUI AND USE_QT_GUI)
    add_subdirectory(core/analysis)
    add_subdirectory(core/automation)
    add_subdirectory(core/cache)
    add_subdirectory(core/database)
    add_subdirectory(core/ext)
    add_subdirectory(core/ux)
endif()

if(ENABLE_GUI AND NOT BUILD_CLI_ONLY)
    add_subdirectory(core/gfx)
    add_subdirectory(core/regcom)
    add_subdirectory(core/vss)
    add_subdirectory(core/rules)
endif()

# C/C++ libraries
add_subdirectory(libs/chash)
add_subdirectory(libs/phash)
add_subdirectory(libs/utils)
add_subdirectory(libs/fuzzyhash)
add_subdirectory(libs/metadata)
add_subdirectory(libs/carving)
add_subdirectory(libs/hidden)
add_subdirectory(libs/slack)
add_subdirectory(libs/mount)
add_subdirectory(libs/logs)
add_subdirectory(libs/yara)
add_subdirectory(libs/clustering)
add_subdirectory(libs/compression)
add_subdirectory(libs/encryption)
add_subdirectory(libs/network)
add_subdirectory(libs/ml)
add_subdirectory(libs/memory)
add_subdirectory(libs/security)

if(ENABLE_GUI AND NOT BUILD_CLI_ONLY)
    add_subdirectory(libs/audfp)
    add_subdirectory(libs/peparse)
    add_subdirectory(libs/lsh)
endif()

# Platform wrappers
add_subdirectory(platform)
add_subdirectory(platform/fswin)
add_subdirectory(platform/util)

# Tests (conditionally enabled)
if(ENABLE_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()

# Display build configuration
message(STATUS "DiskSense64 Build Configuration:")
message(STATUS "  Platform: ${CMAKE_SYSTEM_NAME} (${CMAKE_SYSTEM_PROCESSOR})")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  CLI Only: ${BUILD_CLI_ONLY}")
message(STATUS "  GUI Enabled: ${ENABLE_GUI}")
message(STATUS "  Use Qt GUI: ${USE_QT_GUI}")
message(STATUS "  Testing Enabled: ${ENABLE_TESTING}")
message(STATUS "  Cross-Platform Build: ${CROSS_PLATFORM_BUILD}")

if(PLATFORM_WINDOWS)
    message(STATUS "  Windows Architecture: ${PLATFORM_WINDOWS_X64}")
else()
    message(STATUS "  Linux Architecture: ${PLATFORM_LINUX_X64}")
endif()
