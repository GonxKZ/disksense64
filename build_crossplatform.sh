#!/bin/bash
# Cross-platform build script for DiskSense64 CLI

# Function to print usage
print_usage() {
    echo "Usage: $0 [linux|windows|all]"
    echo "  linux   - Build for Linux x64"
    echo "  windows - Build for Windows x64 (cross-compilation)"
    echo "  all     - Build for both platforms"
}

# Function to build for Linux
build_linux() {
    echo "=== Building for Linux x64 ==="
    
    # Clean previous build
    rm -rf build-linux
    mkdir -p build-linux
    cd build-linux
    
    # Build all object files
    echo "Building object files..."
    
    # Build chash library
    gcc -c ../libs/chash/sha256.c -o sha256.o
    gcc -c ../libs/chash/blake3.c -o blake3.o
    
    # Build phash library
    gcc -c ../libs/phash/phash.c -o phash.o
    
    # Build model
    g++ -c ../core/model/model.cpp -o model.o -std=c++20 -I..
    
    # Build platform fswin
    g++ -c ../platform/fswin/fswin.cpp -o fswin.o -std=c++20 -I..
    
    # Build utils
    g++ -c ../libs/utils/utils.cpp -o utils.o -std=c++20 -I..
    
    # Build index
    g++ -c ../core/index/index.cpp -o index.o -std=c++20 -I..
    g++ -c ../core/index/lsm_index.cpp -o lsm_index.o -std=c++20 -I..
    
    # Build scan
    g++ -c ../core/scan/scan.cpp -o scan.o -std=c++20 -I..
    g++ -c ../core/scan/scanner.cpp -o scanner.o -std=c++20 -I..
    
    # Build ops
    g++ -c ../core/ops/ops.cpp -o ops.o -std=c++20 -I..
    g++ -c ../core/ops/dedupe.cpp -o dedupe.o -std=c++20 -I..
    
    # Create static libraries
    echo "Creating static libraries..."
    ar rcs libchash.a sha256.o blake3.o
    ar rcs libphash.a phash.o
    ar rcs libmodel.a model.o
    ar rcs libfswin.a fswin.o
    ar rcs libutils.a utils.o
    ar rcs libindex.a index.o lsm_index.o
    ar rcs libscan.a scan.o scanner.o
    ar rcs libops.a ops.o dedupe.o
    
    # Build CLI application
    echo "Building CLI application..."
    g++ -c ../apps/DiskSense.Cli/main.cpp -o main.o -std=c++20 -I..
    
    # Link everything together
    g++ -o DiskSense.Cli main.o \
        -L. -lscan -lindex -lops -lutils -lmodel -lfswin -lchash -lphash \
        -lpthread
    
    echo "Linux build completed!"
    ls -la DiskSense.Cli
    
    cd ..
}

# Function to build for Windows (cross-compilation)
build_windows() {
    echo "=== Building for Windows x64 (cross-compilation) ==="
    
    # Check if MinGW-w64 is available
    if ! command -v x86_664-w64-mingw32-gcc &> /dev/null; then
        echo "Error: MinGW-w64 not found. Please install mingw-w64 package."
        return 1
    fi
    
    # Clean previous build
    rm -rf build-windows
    mkdir -p build-windows
    cd build-windows
    
    # Build all object files
    echo "Building object files..."
    
    # Build chash library
    x86_64-w64-mingw32-gcc -c ../libs/chash/sha256.c -o sha256.o
    x86_64-w64-mingw32-gcc -c ../libs/chash/blake3.c -o blake3.o
    
    # Build phash library
    x86_64-w64-mingw32-gcc -c ../libs/phash/phash.c -o phash.o
    
    # Build model
    x86_64-w64-mingw32-g++ -c ../core/model/model.cpp -o model.o -std=c++20 -I..
    
    # Build platform fswin
    x86_64-w64-mingw32-g++ -c ../platform/fswin/fswin.cpp -o fswin.o -std=c++20 -I..
    
    # Build utils
    x86_64-w64-mingw32-g++ -c ../libs/utils/utils.cpp -o utils.o -std=c++20 -I..
    
    # Build index
    x86_64-w64-mingw32-g++ -c ../core/index/index.cpp -o index.o -std=c++20 -I..
    x86_64-w64-mingw32-g++ -c ../core/index/lsm_index.cpp -o lsm_index.o -std=c++20 -I..
    
    # Build scan
    x86_64-w64-mingw32-g++ -c ../core/scan/scan.cpp -o scan.o -std=c++20 -I..
    x86_64-w64-mingw32-g++ -c ../core/scan/scanner.cpp -o scanner.o -std=c++20 -I..
    
    # Build ops
    x86_64-w64-mingw32-g++ -c ../core/ops/ops.cpp -o ops.o -std=c++20 -I..
    x86_64-w64-mingw32-g++ -c ../core/ops/dedupe.cpp -o dedupe.o -std=c++20 -I..
    
    # Create static libraries
    echo "Creating static libraries..."
    x86_64-w64-mingw32-ar rcs libchash.a sha256.o blake3.o
    x86_64-w64-mingw32-ar rcs libphash.a phash.o
    x86_64-w64-mingw32-ar rcs libmodel.a model.o
    x86_64-w64-mingw32-ar rcs libfswin.a fswin.o
    x86_64-w64-mingw32-ar rcs libutils.a utils.o
    x86_64-w64-mingw32-ar rcs libindex.a index.o lsm_index.o
    x86_64-w64-mingw32-ar rcs libscan.a scan.o scanner.o
    x86_64-w64-mingw32-ar rcs libops.a ops.o dedupe.o
    
    # Build CLI application
    echo "Building CLI application..."
    x86_64-w64-mingw32-g++ -c ../apps/DiskSense.Cli/main.cpp -o main.o -std=c++20 -I..
    
    # Link everything together
    x86_64-w64-mingw32-g++ -o DiskSense.Cli.exe main.o \
        -L. -lscan -lindex -lops -lutils -lmodel -lfswin -lchash -lphash \
        -static -lssp
    
    echo "Windows build completed!"
    ls -la DiskSense.Cli.exe
    
    cd ..
}

# Main script
if [ $# -eq 0 ]; then
    print_usage
    exit 1
fi

case "$1" in
    linux)
        build_linux
        ;;
    windows)
        build_windows
        ;;
    all)
        build_linux
        build_windows
        ;;
    *)
        print_usage
        exit 1
        ;;
esac