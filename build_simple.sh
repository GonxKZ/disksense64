#!/bin/bash
# Simple build script for DiskSense64 CLI

# Clean previous build
rm -rf build-simple
mkdir -p build-simple
cd build-simple

# Build libraries
echo "Building libraries..."

# Build chash library
gcc -c ../libs/chash/sha256.c -o sha256.o
gcc -c ../libs/chash/blake3.c -o blake3.o
ar rcs libchash.a sha256.o blake3.o

# Build phash library
gcc -c ../libs/phash/phash.c -o phash.o
ar rcs libphash.a phash.o

# Build utils library
g++ -c ../libs/utils/utils.cpp -o utils.o -I..
ar rcs libutils.a utils.o

# Build core libraries
echo "Building core libraries..."

# Build core model
g++ -c ../core/model/model.cpp -o model.o -I..
ar rcs libmodel.a model.o

# Build platform fswin
g++ -c ../platform/fswin/fswin.cpp -o fswin.o -I..
ar rcs libfswin.a fswin.o

# Build core scan with BLAKE3 source files included directly
g++ -c ../core/scan/scan.cpp -o scan.o -I..
g++ -c ../core/scan/scanner.cpp -o scanner.o -I..
# Include BLAKE3 source files directly to avoid linking issues
gcc -c ../libs/chash/blake3.c -o blake3_scan.o
gcc -c ../libs/chash/sha256.c -o sha256_scan.o
ar rcs libscan.a scan.o scanner.o blake3_scan.o sha256_scan.o

# Build core index
g++ -c ../core/index/index.cpp -o index.o -I..
g++ -c ../core/index/lsm_index.cpp -o lsm_index.o -I..
ar rcs libindex.a index.o lsm_index.o

# Build core ops
g++ -c ../core/ops/ops.cpp -o ops.o -I..
g++ -c ../core/ops/dedupe.cpp -o dedupe.o -I..
ar rcs libops.a ops.o dedupe.o

# Build CLI application
echo "Building CLI application..."
g++ -c ../apps/DiskSense.Cli/main.cpp -o main.o -I..
g++ -o DiskSense.Cli main.o -L. -lchash -lphash -lutils -lmodel -lfswin -lscan -lindex -lops -lpthread

echo "Build completed!"
ls -la DiskSense.Cli