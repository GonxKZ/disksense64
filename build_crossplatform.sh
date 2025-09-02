#!/bin/bash
# Comprehensive cross-platform build script for DiskSense64

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print status messages
print_status() {
    echo -e "${GREEN}[STATUS]${NC} $1"
}

# Function to print warnings
print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

# Function to print errors
print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to print info
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

# Function to check if a command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to detect OS
detect_os() {
    case "$(uname -s)" in
        Linux*)     echo "Linux";;
        Darwin*)    echo "Mac";;
        CYGWIN*)    echo "Cygwin";;
        MINGW*)     echo "MinGW";;
        *)          echo "Unknown";;
    esac
}

# Function to detect architecture
detect_arch() {
    case "$(uname -m)" in
        x86_64)     echo "x64";;
        i386|i686)  echo "x86";;
        armv7l)     echo "arm";;
        aarch64)    echo "arm64";;
        *)          echo "unknown";;
    esac
}

# Function to check prerequisites
check_prerequisites() {
    print_status "Checking prerequisites..."
    
    # Check for CMake
    if ! command_exists cmake; then
        print_error "CMake is not installed. Please install CMake 3.20 or later."
        return 1
    fi
    
    # Check CMake version
    CMAKE_VERSION=$(cmake --version | head -n1 | awk '{print $3}')
    print_info "CMake version: $CMAKE_VERSION"
    
    # Check for build tools
    if command_exists ninja; then
        BUILD_TOOL="Ninja"
        print_info "Using Ninja build system"
    elif command_exists make; then
        BUILD_TOOL="Make"
        print_info "Using Make build system"
    else
        print_error "Neither Ninja nor Make is installed. Please install one of them."
        return 1
    fi
    
    # Check for compiler
    if command_exists g++; then
        COMPILER="GCC"
        COMPILER_VERSION=$(g++ --version | head -n1 | awk '{print $3}')
        print_info "Using GCC version: $COMPILER_VERSION"
    elif command_exists clang++; then
        COMPILER="Clang"
        COMPILER_VERSION=$(clang++ --version | head -n1 | awk '{print $3}')
        print_info "Using Clang version: $COMPILER_VERSION"
    else
        print_error "No C++ compiler found. Please install GCC or Clang."
        return 1
    fi
    
    return 0
}

# Function to check cross-compilation tools
check_cross_compilation_tools() {
    print_status "Checking cross-compilation tools..."
    
    # Check for MinGW-w64
    if command_exists x86_64-w64-mingw32-g++; then
        print_info "MinGW-w64 found for Windows cross-compilation"
        HAS_MINGW64=true
    else
        print_warning "MinGW-w64 not found. Windows cross-compilation will not be available."
        HAS_MINGW64=false
    fi
    
    # Check for Linux cross-compilation tools
    if command_exists x86_64-linux-gnu-g++; then
        print_info "Linux cross-compilation tools found"
        HAS_LINUX_CROSS=true
    else
        print_warning "Linux cross-compilation tools not found."
        HAS_LINUX_CROSS=false
    fi
    
    return 0
}

# Function to build for native platform
build_native() {
    local build_type=${1:-Release}
    local build_dir=${2:-build-native}
    
    print_status "Building for native platform ($build_type)..."
    
    # Create build directory
    rm -rf "$build_dir"
    mkdir -p "$build_dir"
    cd "$build_dir"
    
    # Configure with CMake
    if [ "$BUILD_TOOL" = "Ninja" ]; then
        cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE="$build_type"
    else
        cmake .. -DCMAKE_BUILD_TYPE="$build_type"
    fi
    
    if [ $? -ne 0 ]; then
        print_error "CMake configuration failed"
        cd ..
        return 1
    fi
    
    # Build
    if [ "$BUILD_TOOL" = "Ninja" ]; then
        ninja -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
    else
        make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
    fi
    
    if [ $? -ne 0 ]; then
        print_error "Build failed"
        cd ..
        return 1
    fi
    
    print_status "Native build completed successfully!"
    cd ..
    return 0
}

# Function to build for Windows (cross-compilation)
build_windows() {
    local build_type=${1:-Release}
    local arch=${2:-x64}
    local build_dir=${3:-build-win-$arch}
    
    if [ "$HAS_MINGW64" = false ]; then
        print_warning "Skipping Windows build - MinGW-w64 not available"
        return 0
    fi
    
    print_status "Building for Windows $arch ($build_type)..."
    
    # Create build directory
    rm -rf "$build_dir"
    mkdir -p "$build_dir"
    cd "$build_dir"
    
    # Configure with CMake for cross-compilation
    if [ "$BUILD_TOOL" = "Ninja" ]; then
        cmake .. -G "Ninja" -DCMAKE_TOOLCHAIN_FILE="../cmake/mingw64.cmake" -DCMAKE_BUILD_TYPE="$build_type"
    else
        cmake .. -DCMAKE_TOOLCHAIN_FILE="../cmake/mingw64.cmake" -DCMAKE_BUILD_TYPE="$build_type"
    fi
    
    if [ $? -ne 0 ]; then
        print_error "CMake configuration failed for Windows $arch"
        cd ..
        return 1
    fi
    
    # Build
    if [ "$BUILD_TOOL" = "Ninja" ]; then
        ninja -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
    else
        make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
    fi
    
    if [ $? -ne 0 ]; then
        print_error "Windows $arch build failed"
        cd ..
        return 1
    fi
    
    print_status "Windows $arch build completed successfully!"
    cd ..
    return 0
}

# Function to build for Linux (cross-compilation)
build_linux() {
    local build_type=${1:-Release}
    local arch=${2:-x64}
    local build_dir=${3:-build-linux-$arch}
    
    if [ "$HAS_LINUX_CROSS" = false ]; then
        print_warning "Skipping Linux build - cross-compilation tools not available"
        return 0
    fi
    
    print_status "Building for Linux $arch ($build_type)..."
    
    # Create build directory
    rm -rf "$build_dir"
    mkdir -p "$build_dir"
    cd "$build_dir"
    
    # Configure with CMake for cross-compilation
    if [ "$BUILD_TOOL" = "Ninja" ]; then
        cmake .. -G "Ninja" -DCMAKE_TOOLCHAIN_FILE="../cmake/linux64.cmake" -DCMAKE_BUILD_TYPE="$build_type"
    else
        cmake .. -DCMAKE_TOOLCHAIN_FILE="../cmake/linux64.cmake" -DCMAKE_BUILD_TYPE="$build_type"
    fi
    
    if [ $? -ne 0 ]; then
        print_error "CMake configuration failed for Linux $arch"
        cd ..
        return 1
    fi
    
    # Build
    if [ "$BUILD_TOOL" = "Ninja" ]; then
        ninja -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
    else
        make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
    fi
    
    if [ $? -ne 0 ]; then
        print_error "Linux $arch build failed"
        cd ..
        return 1
    fi
    
    print_status "Linux $arch build completed successfully!"
    cd ..
    return 0
}

# Function to run tests
run_tests() {
    local build_type=${1:-Debug}
    local build_dir=${2:-build-test}
    
    print_status "Running tests ($build_type)..."
    
    # Create build directory
    rm -rf "$build_dir"
    mkdir -p "$build_dir"
    cd "$build_dir"
    
    # Configure with CMake for testing
    if [ "$BUILD_TOOL" = "Ninja" ]; then
        cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE="$build_type" -DENABLE_TESTING=ON
    else
        cmake .. -DCMAKE_BUILD_TYPE="$build_type" -DENABLE_TESTING=ON
    fi
    
    if [ $? -ne 0 ]; then
        print_error "CMake configuration failed for tests"
        cd ..
        return 1
    fi
    
    # Build tests
    if [ "$BUILD_TOOL" = "Ninja" ]; then
        ninja -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
    else
        make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
    fi
    
    if [ $? -ne 0 ]; then
        print_error "Test build failed"
        cd ..
        return 1
    fi
    
    # Run tests
    if [ "$BUILD_TOOL" = "Ninja" ]; then
        ninja test
    else
        make test
    fi
    
    if [ $? -ne 0 ]; then
        print_error "Some tests failed"
        cd ..
        return 1
    fi
    
    print_status "All tests passed!"
    cd ..
    return 0
}

# Function to create packages
create_packages() {
    print_status "Creating distribution packages..."
    
    # Create dist directory
    rm -rf dist
    mkdir -p dist
    
    # Package native build
    if [ -d "build-native" ]; then
        mkdir -p dist/native
        cp -r build-native/bin/* dist/native/ 2>/dev/null || true
        cp README.md dist/native/ 2>/dev/null || true
        cp LICENSE dist/native/ 2>/dev/null || true
        cd dist
        if [ "$(detect_os)" = "Linux" ]; then
            tar -czf DiskSense64-native-linux-$(detect_arch).tar.gz native
            print_info "Native Linux package created: dist/DiskSense64-native-linux-$(detect_arch).tar.gz"
        else
            zip -r DiskSense64-native-windows-$(detect_arch).zip native
            print_info "Native Windows package created: dist/DiskSense64-native-windows-$(detect_arch).zip"
        fi
        cd ..
    fi
    
    # Package Windows build
    if [ -d "build-win-x64" ]; then
        mkdir -p dist/windows-x64
        cp -r build-win-x64/bin/* dist/windows-x64/ 2>/dev/null || true
        cp README.md dist/windows-x64/ 2>/dev/null || true
        cp LICENSE dist/windows-x64/ 2>/dev/null || true
        cd dist
        zip -r DiskSense64-windows-x64.zip windows-x64
        print_info "Windows x64 package created: dist/DiskSense64-windows-x64.zip"
        cd ..
    fi
    
    # Package Linux build
    if [ -d "build-linux-x64" ]; then
        mkdir -p dist/linux-x64
        cp -r build-linux-x64/bin/* dist/linux-x64/ 2>/dev/null || true
        cp README.md dist/linux-x64/ 2>/dev/null || true
        cp LICENSE dist/linux-x64/ 2>/dev/null || true
        cd dist
        tar -czf DiskSense64-linux-x64.tar.gz linux-x64
        print_info "Linux x64 package created: dist/DiskSense64-linux-x64.tar.gz"
        cd ..
    fi
    
    print_status "Distribution packages created successfully!"
    return 0
}

# Function to clean build directories
clean_builds() {
    print_status "Cleaning build directories..."
    rm -rf build-* dist
    print_status "Clean completed!"
    return 0
}

# Main script
main() {
    local os=$(detect_os)
    local arch=$(detect_arch)
    
    print_status "DiskSense64 Cross-Platform Build Script"
    print_status "======================================"
    print_info "Detected OS: $os"
    print_info "Detected Architecture: $arch"
    echo
    
    # Check prerequisites
    if ! check_prerequisites; then
        exit 1
    fi
    
    # Check cross-compilation tools
    check_cross_compilation_tools
    
    # Parse command line arguments
    case "$1" in
        native)
            build_native "${2:-Release}" "${3:-build-native}"
            ;;
        win|windows)
            build_windows "${2:-Release}" "x64" "${3:-build-win-x64}"
            ;;
        linux)
            build_linux "${2:-Release}" "x64" "${3:-build-linux-x64}"
            ;;
        all)
            build_native "${2:-Release}" "${3:-build-native}"
            build_windows "${2:-Release}" "x64" "${4:-build-win-x64}"
            build_linux "${2:-Release}" "x64" "${5:-build-linux-x64}"
            ;;
        test|tests)
            run_tests "${2:-Debug}" "${3:-build-test}"
            ;;
        package|dist)
            create_packages
            ;;
        clean)
            clean_builds
            ;;
        *)
            echo "Usage: $0 {native|win|linux|all|test|package|clean} [build_type] [build_dir]"
            echo
            echo "Commands:"
            echo "  native    - Build for native platform"
            echo "  win       - Build for Windows (cross-compilation)"
            echo "  linux     - Build for Linux (cross-compilation)"
            echo "  all       - Build for all platforms"
            echo "  test      - Run unit tests"
            echo "  package   - Create distribution packages"
            echo "  clean     - Clean build directories"
            echo
            echo "Arguments:"
            echo "  build_type - Release, Debug, RelWithDebInfo, MinSizeRel (default: Release)"
            echo "  build_dir  - Build directory name (default: build-{platform})"
            echo
            exit 1
            ;;
    esac
}

# Run main function
main "$@"