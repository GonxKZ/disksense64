# MinGW-w64 cross-compilation toolchain file
# This file configures the build for Windows targets from Linux

# Set the system name and processor
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Set the cross-compilation toolchain prefix
set(TOOLCHAIN_PREFIX x86_64-w64-mingw32)

# Set the compilers
set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}-gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}-g++)
set(CMAKE_RC_COMPILER ${TOOLCHAIN_PREFIX}-windres)

# Set the target environment
set(CMAKE_FIND_ROOT_PATH /usr/${TOOLCHAIN_PREFIX})

# Modify default behavior of FIND_XXX() commands
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Set Windows-specific flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_WIN32_WINNT=0x0A00") # Windows 10
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUNICODE -D_UNICODE")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mwindows") # Windows subsystem

# Set linker flags
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--stack,8388608") # 8MB stack
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--subsystem,windows")

# Set compiler-specific flags
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fstack-protector-strong")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_FORTIFY_SOURCE=2")
endif()

# Define platform macros
add_definitions(-DWIN32)
add_definitions(-D_WINDOWS)
add_definitions(-DNOMINMAX)
add_definitions(-DWINVER=0x0A00)
add_definitions(-D_WIN32_WINNT=0x0A00)

# Set Windows libraries
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
    d2d1
    dwrite
    dcomp
    windowscodecs
)

# Link Windows libraries
link_libraries(${WIN32_LIBS})

# Set include directories
include_directories(/usr/${TOOLCHAIN_PREFIX}/include)

# Set library directories
link_directories(/usr/${TOOLCHAIN_PREFIX}/lib)

# Platform-specific settings
set(PLATFORM_WINDOWS ON)
set(PLATFORM_WINDOWS_X64 ON)
set(CROSS_PLATFORM_BUILD ON)

# Message to confirm toolchain is loaded
message(STATUS "Loaded MinGW-w64 cross-compilation toolchain")
message(STATUS "Target: Windows x64")
message(STATUS "Toolchain prefix: ${TOOLCHAIN_PREFIX}")