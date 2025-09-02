#!/bin/bash
# Manual build script for DiskSense64 CLI

# Clean previous build
rm -rf build-manual
mkdir -p build-manual
cd build-manual

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
# Note the order: dependencies first, then the libraries that depend on them
g++ -o DiskSense.Cli main.o \
    -L. -lscan -lindex -lops -lutils -lmodel -lfswin -lchash -lphash \
    -lpthread

echo "Build completed!"
ls -la DiskSense.Cli