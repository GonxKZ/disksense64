#!/bin/bash
# Build script for DiskSense64

# Create build directory if it doesn't exist
mkdir -p build
cd build

# Configure the project with CMake
echo "Configuring project with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build the project
echo "Building project..."
make -j$(nproc)

# Check if build was successful
if [ $? -eq 0 ]; then
    echo "Build successful!"
    echo "Binaries are located in build/bin/"
else
    echo "Build failed!"
    exit 1
fi