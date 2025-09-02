#!/bin/bash
# Multi-platform build script for DiskSense64

# Get the absolute path to the project directory
PROJECT_DIR=$(pwd)

# Function to build for Windows (cross-compilation)
build_windows() {
    echo "Building for Windows..."
    
    # Create build directory
    rm -rf build-win
    mkdir -p build-win
    cd build-win
    
    # Configure with CMake for MinGW-w64 cross-compilation
    cmake .. -DCMAKE_TOOLCHAIN_FILE="$PROJECT_DIR/cmake/mingw64.cmake" -DCMAKE_BUILD_TYPE=Release -DBUILD_CLI_ONLY=ON
    
    # Build the project
    make -j$(nproc) DiskSense.Cli
    
    echo "Windows build completed!"
    echo "CLI executable located at: build-win/bin/DiskSense.Cli.exe"
    
    cd ..
}

# Function to build for Linux
build_linux() {
    echo "Building for Linux..."
    
    # Create build directory
    rm -rf build-linux
    mkdir -p build-linux
    cd build-linux
    
    # Configure with CMake
    cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_CLI_ONLY=ON
    
    # Build the project
    make -j$(nproc) DiskSense.Cli
    
    echo "Linux build completed!"
    echo "CLI executable located at: build-linux/bin/DiskSense.Cli"
    
    cd ..
}

# Function to build for native Windows
build_windows_native() {
    echo "Building for native Windows..."
    
    # Create build directory
    rm -rf build-win-native
    mkdir -p build-win-native
    cd build-win-native
    
    # Configure with CMake
    cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DBUILD_CLI_ONLY=ON
    
    # Build the project
    mingw32-make -j$(nproc) DiskSense.Cli
    
    echo "Native Windows build completed!"
    echo "CLI executable located at: build-win-native/bin/DiskSense.Cli.exe"
    
    cd ..
}

# Main script
case "$1" in
    win|windows)
        build_windows
        ;;
    linux)
        build_linux
        ;;
    win-native)
        build_windows_native
        ;;
    all)
        build_windows
        build_linux
        build_windows_native
        ;;
    *)
        echo "Usage: $0 {win|linux|win-native|all}"
        echo "  win         - Build for Windows (cross-compilation)"
        echo "  linux       - Build for Linux"
        echo "  win-native  - Build for Windows (native)"
        echo "  all         - Build for all platforms"
        exit 1
        ;;
esac