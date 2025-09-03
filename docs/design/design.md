# DiskSense64 - Disk Analysis Suite

## Overview

DiskSense64 is a comprehensive disk analysis suite for Windows 11 that provides four core capabilities:

1. **Exact File Deduplication** with NTFS hardlinks
2. **Disk Space Visualization** with treemap charts
3. **Residue Detection and Cleanup** of orphaned files and registry entries
4. **Perceptual Duplicate Detection** for images and audio

## Architecture

The application follows a modular architecture with the following components:

### Core Modules

- **Scanner**: File system traversal using Win32 APIs or MFT reader
- **Index**: LSM-based persistent index with memory mapping
- **Engine**: IOCP-based concurrent I/O scheduler
- **Ops**: File operations including deduplication and cleanup
- **GFX**: Treemap visualization using Direct2D
- **RegCom**: Registry and COM component analysis
- **USN**: USN Journal monitoring for incremental updates
- **VSS**: Volume Shadow Copy for safe operations
- **Rules**: Heuristics for residue detection

### Libraries

- **chash**: Cryptographic hashing (SHA-256, BLAKE3)
- **phash**: Perceptual hashing for images
- **audfp**: Audio fingerprinting
- **peparse**: PE file parser
- **lsh**: Locality-sensitive hashing for similarity search

### Platform Wrappers

- **fswin**: Windows file system abstractions
- **util**: Utility functions and helpers

## Design Principles

1. **I/O Optimization**: Minimize disk seeks through sequential reading and IOCP
2. **Revertibility**: All operations can be undone through VSS or recycle bin
3. **Transactionality**: Operations are idempotent and atomic
4. **Memory Efficiency**: Memory-mapped files and streaming processing
5. **No External Dependencies**: Pure Win32/COM implementation

## Building

### Prerequisites

- CMake 3.20 or later
- Ninja build system
- MinGW-w64 toolchain (for cross-compilation from WSL)
- Windows 11 SDK

### Building from WSL

```bash
./build.sh
```

### Building from Windows

```cmd
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

## Usage

### Command Line Interface

```bash
# Scan a directory and build index
DiskSense.Cli.exe scan C:\Users\Username\Documents

# Find duplicates (simulation mode)
DiskSense.Cli.exe dedupe C:\Users\Username\Documents

# Generate treemap (GUI only)
DiskSense.Gui.exe treemap C:\Users\Username\Documents
```

## Modules in Detail

### 1. Deduplication Module

- **Size-based filtering**: O(1) elimination of unique-sized files
- **Head/Tail signatures**: 32KB fingerprint for fast candidate filtering
- **Full hash verification**: BLAKE3/SHA-256 for final verification
- **Content-defined chunking**: Detects shifted/inserted duplicates
- **Hardlink support**: NTFS hardlinks for space savings
- **Safety features**: Simulation mode, VSS snapshots, recycle bin

### 2. Treemap Visualization

- **Squarified algorithm**: Minimizes rectangle aspect ratios
- **Direct2D rendering**: GPU-accelerated visualization
- **Interactive navigation**: Zoom, pan, and selection
- **Hierarchical layout**: Directory structure visualization
- **Color coding**: By file type, size, or owner

### 3. Residue Detection

- **Installation tracking**: Uninstall entries, App Paths, COM registrations
- **Reference counting**: PE imports for DLL dependency analysis
- **Path heuristics**: Orphaned files in known application directories
- **Registry cleanup**: Broken COM entries, missing file references
- **Confidence scoring**: Risk assessment for each cleanup action

### 4. Perceptual Similarity

- **Image pHash**: 32×32 DCT-based perceptual hashing
- **Audio fingerprinting**: Chromatic energy with min-hash
- **LSH indexing**: Fast approximate nearest neighbor search
- **Similarity thresholds**: Adjustable tolerance levels
- **Preview features**: Blink comparison, audio playback

## Performance Targets

- **File scanning**: ≥ 200-400k files/minute on NVMe SSD
- **Hashing**: ≥ 1.5 GB/s BLAKE3, ≥ 0.6 GB/s SHA-256
- **pHash**: ≥ 20k images/minute with SSE/AVX
- **Treemap**: ≤ 16ms render time for 60 FPS
- **I/O scheduling**: Adaptive concurrency based on latency

## Security Features

- **asInvoker execution**: No mandatory elevation
- **Elevated operations**: Puntual privilege escalation when needed
- **ACL respect**: File access respects security permissions
- **Share modes**: Non-exclusive file access to avoid conflicts
- **Transaction safety**: All operations can be rolled back