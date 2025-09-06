# Compiler warnings and security flags
if(MSVC)
    # MSVC-specific flags
    add_compile_options(
        /W4           # Warning level 4
        /permissive-  # Standard-conforming C++
        /EHsc         # C++ exception handling model
        /GS           # Buffer security check
        /guard:cf     # Control Flow Guard
    )
    
    # Optimization flags
    if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        add_compile_options(/O2)  # Full optimization
    endif()
    
    # Security flags
    add_compile_options(
        /DYNAMICBASE  # Address Space Layout Randomization
        /NXCOMPAT     # Data Execution Prevention
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    # Clang/GCC-specific flags
    add_compile_options(
        -Wall
        -Wextra
    )
    
    # Optimization flags
    if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        add_compile_options(-O2)
    endif()
    
    # Security flags for MinGW - only apply when compiling natively on Windows
    # The -mguard=cf flag is not supported in cross-compilation
    if(WIN32 AND NOT CMAKE_CROSSCOMPILING)
        add_compile_options(
            -mguard=cf
        )
    endif()
endif()

# Debug-specific flags
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(MSVC)
        add_compile_options(/Od)  # Disable optimization
    else()
        add_compile_options(-O0)  # Disable optimization
    endif()
endif()