#!/bin/bash
# Complete build script for DiskSense64

set -e  # Exit on any error

echo "==========================================="
echo "   DiskSense64 - Complete Build Script     "
echo "==========================================="

# Function to print section headers
print_header() {
    echo ""
    echo "-------------------------------------------"
    echo "  $1"
    echo "-------------------------------------------"
}

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Check prerequisites
print_header "Checking Prerequisites"

if ! command_exists cmake; then
    echo "ERROR: CMake is not installed"
    exit 1
fi

if ! command_exists git; then
    echo "ERROR: Git is not installed"
    exit 1
fi

echo "✓ CMake found"
echo "✓ Git found"

# Determine build system
BUILD_SYSTEM="Unix Makefiles"
if command_exists ninja; then
    BUILD_SYSTEM="Ninja"
    echo "✓ Ninja found (will use for faster builds)"
fi

# Determine number of jobs
JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
echo "Using $JOBS parallel jobs for compilation"

# Create build directory
print_header "Setting Up Build Environment"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

if [ ! -d "$BUILD_DIR" ]; then
    mkdir -p "$BUILD_DIR"
    echo "Created build directory: $BUILD_DIR"
fi

cd "$BUILD_DIR"

# Configure project
print_header "Configuring Project"

echo "Using build system: $BUILD_SYSTEM"
echo "Build type: Release"

cmake .. \
    -G "$BUILD_SYSTEM" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$SCRIPT_DIR/install" \
    -DENABLE_PYTHON=ON \
    -DENABLE_LUA=ON \
    -DENABLE_TESTS=ON

# Build project
print_header "Building Project"

if [ "$BUILD_SYSTEM" = "Ninja" ]; then
    ninja -j"$JOBS"
else
    make -j"$JOBS"
fi

echo "✓ Build completed successfully"

# Run tests
print_header "Running Tests"

if [ "$BUILD_SYSTEM" = "Ninja" ]; then
    ninja test
else
    make test
fi

echo "✓ All tests passed"

# Install project
print_header "Installing Project"

if [ "$BUILD_SYSTEM" = "Ninja" ]; then
    ninja install
else
    make install
fi

echo "✓ Installation completed"

# Create package
print_header "Creating Distribution Package"

PACKAGE_NAME="DiskSense64-$(date +%Y%m%d)-$(uname -s)-$(uname -m)"
PACKAGE_DIR="$SCRIPT_DIR/packages/$PACKAGE_NAME"

mkdir -p "$PACKAGE_DIR"

# Copy binaries
cp -r "$SCRIPT_DIR/install/bin" "$PACKAGE_DIR/"
cp -r "$SCRIPT_DIR/install/lib" "$PACKAGE_DIR/"
cp -r "$SCRIPT_DIR/install/include" "$PACKAGE_DIR/"
cp -r "$SCRIPT_DIR/install/share" "$PACKAGE_DIR/"

# Copy documentation
cp "$SCRIPT_DIR/README.md" "$PACKAGE_DIR/"
cp "$SCRIPT_DIR/LICENSE" "$PACKAGE_DIR/"
cp "$SCRIPT_DIR/CHANGELOG.md" "$PACKAGE_DIR/" 2>/dev/null || true

# Create package
cd "$SCRIPT_DIR/packages"
tar -czf "$PACKAGE_NAME.tar.gz" "$PACKAGE_NAME"

echo "Package created: $SCRIPT_DIR/packages/$PACKAGE_NAME.tar.gz"

# Cleanup
print_header "Cleaning Up"

rm -rf "$PACKAGE_DIR"

# Print completion message
print_header "Build Complete!"

echo "DiskSense64 has been successfully built and packaged."
echo ""
echo "Binaries are located in: $SCRIPT_DIR/install/bin"
echo "Package: $SCRIPT_DIR/packages/$PACKAGE_NAME.tar.gz"
echo ""
echo "To run the application:"
echo "  cd $SCRIPT_DIR/install/bin"
echo "  ./DiskSense.Gui  # For graphical interface"
echo "  ./DiskSense.Cli  # For command-line interface"
echo ""
echo "Enjoy using DiskSense64!"

exit 0