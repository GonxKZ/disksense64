#!/bin/bash
# Comprehensive build and test script for DiskSense64

# Get the absolute path to the project directory
PROJECT_DIR=$(pwd)

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print status messages
print_status() {
    echo -e "${GREEN}[STATUS]${NC} $1"
}

# Function to print warning messages
print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

# Function to print error messages
print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to build for Windows (cross-compilation)
build_windows() {
    print_status "Building for Windows (cross-compilation)..."
    
    # Create build directory
    rm -rf build-win
    mkdir -p build-win
    cd build-win
    
    # Configure with CMake for MinGW-w64 cross-compilation
    cmake .. -DCMAKE_TOOLCHAIN_FILE="$PROJECT_DIR/cmake/mingw64.cmake" -DCMAKE_BUILD_TYPE=Release -DBUILD_CLI_ONLY=OFF
    
    if [ $? -ne 0 ]; then
        print_error "CMake configuration failed for Windows"
        cd ..
        return 1
    fi
    
    # Build the project
    make -j$(nproc) DiskSense.Cli
    make -j$(nproc) DiskSense.Gui
    
    if [ $? -ne 0 ]; then
        print_error "Build failed for Windows"
        cd ..
        return 1
    fi
    
    print_status "Windows build completed successfully!"
    print_status "CLI executable located at: build-win/bin/DiskSense.Cli.exe"
    
    cd ..
    return 0
}

# Function to build for Linux
build_linux() {
    print_status "Building for Linux..."
    
    # Create build directory
    rm -rf build-linux
    mkdir -p build-linux
    cd build-linux
    
    # Configure with CMake
    cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_CLI_ONLY=OFF
    
    if [ $? -ne 0 ]; then
        print_error "CMake configuration failed for Linux"
        cd ..
        return 1
    fi
    
    # Build the project
    make -j$(nproc) DiskSense.Cli
    make -j$(nproc) DiskSense.Gui
    
    if [ $? -ne 0 ]; then
        print_error "Build failed for Linux"
        cd ..
        return 1
    fi
    
    print_status "Linux build completed successfully!"
    print_status "CLI executable located at: build-linux/bin/DiskSense.Cli"
    
    cd ..
    return 0
}

# Function to build for native Windows
build_windows_native() {
    print_status "Building for native Windows..."
    
    # Check if we're on Windows
    if [[ "$OSTYPE" != "msys" && "$OSTYPE" != "win32" ]]; then
        print_warning "This script must be run on Windows to build native Windows binaries"
        return 1
    fi
    
    # Create build directory
    rm -rf build-win-native
    mkdir -p build-win-native
    cd build-win-native
    
    # Configure with CMake
    cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DBUILD_CLI_ONLY=OFF
    
    if [ $? -ne 0 ]; then
        print_error "CMake configuration failed for native Windows"
        cd ..
        return 1
    fi
    
    # Build the project
    mingw32-make -j$(nproc) DiskSense.Cli
    mingw32-make -j$(nproc) DiskSense.Gui
    
    if [ $? -ne 0 ]; then
        print_error "Build failed for native Windows"
        cd ..
        return 1
    fi
    
    print_status "Native Windows build completed successfully!"
    print_status "CLI executable located at: build-win-native/bin/DiskSense.Cli.exe"
    
    cd ..
    return 0
}

# Function to run tests
run_tests() {
    print_status "Running unit tests..."
    
    # Create test build directory
    rm -rf build-test
    mkdir -p build-test
    cd build-test
    
    # Configure with CMake
    cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_CLI_ONLY=ON
    
    if [ $? -ne 0 ]; then
        print_error "CMake configuration failed for tests"
        cd ..
        return 1
    fi
    
    # Build tests
    make -j$(nproc)
    
    if [ $? -ne 0 ]; then
        print_error "Build failed for tests"
        cd ..
        return 1
    fi
    
    # Run tests
    ctest --output-on-failure
    
    if [ $? -ne 0 ]; then
        print_error "Some tests failed"
        cd ..
        return 1
    fi
    
    print_status "All tests passed!"
    
    cd ..
    return 0
}

# Function to create distribution packages
create_packages() {
    print_status "Creating distribution packages..."
    
    # Create dist directory
    rm -rf dist
    mkdir -p dist
    
    # Package Windows build
    if [ -f "build-win/bin/DiskSense.Cli.exe" ]; then
        mkdir -p dist/windows-x64
        cp build-win/bin/DiskSense.Cli.exe dist/windows-x64/
        cp README.md dist/windows-x64/
        cp LICENSE dist/windows-x64/ 2>/dev/null || touch dist/windows-x64/LICENSE
        cd dist
        zip -r DiskSense64-windows-x64.zip windows-x64
        cd ..
        print_status "Windows x64 package created: dist/DiskSense64-windows-x64.zip"
    fi
    
    # Package Linux build
    if [ -f "build-linux/bin/DiskSense.Cli" ]; then
        mkdir -p dist/linux-x64
        cp build-linux/bin/DiskSense.Cli dist/linux-x64/
        cp README.md dist/linux-x64/
        cp LICENSE dist/linux-x64/ 2>/dev/null || touch dist/linux-x64/LICENSE
        cd dist
        tar -czf DiskSense64-linux-x64.tar.gz linux-x64
        cd ..
        print_status "Linux x64 package created: dist/DiskSense64-linux-x64.tar.gz"
    fi
    
    print_status "Distribution packages created successfully!"
    return 0
}

# Main script
print_status "DiskSense64 Build and Test Script"
print_status "=================================="

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
    test|tests)
        run_tests
        ;;
    package|dist)
        create_packages
        ;;
    all)
        build_windows
        build_linux
        run_tests
        create_packages
        ;;
    clean)
        print_status "Cleaning build directories..."
        rm -rf build-win build-linux build-win-native build-test dist
        print_status "Clean completed!"
        ;;
    *)
        echo "Usage: $0 {win|linux|win-native|test|package|all|clean}"
        echo "  win         - Build for Windows (cross-compilation)"
        echo "  linux       - Build for Linux"
        echo "  win-native  - Build for Windows (native)"
        echo "  test        - Run unit tests"
        echo "  package     - Create distribution packages"
        echo "  all         - Build for all platforms, run tests, and create packages"
        echo "  clean       - Clean build directories"
        exit 1
        ;;
esac