# DiskSense64 - Cross-Platform Disk Analysis Suite

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen)](https://github.com/yourusername/disksense64)
[![License](https://img.shields.io/badge/license-MIT-blue)](https://opensource.org/licenses/MIT)
[![Platforms](https://img.shields.io/badge/platforms-Windows%20%7C%20Linux-lightgrey)](https://github.com/yourusername/disksense64)

A comprehensive, cross-platform disk analysis suite that provides four core capabilities:
1. **Exact File Deduplication** with hardlinks
2. **Disk Space Visualization** with treemap charts
3. **Residue Detection and Cleanup** of orphaned files
4. **Perceptual Duplicate Detection** for images and audio

## 🌟 Features

### 🔍 **Exact File Deduplication**
- **Multi-stage filtering** for maximum efficiency:
  - Size-based filtering (O(1) elimination)
  - Head/Tail signatures (fast candidate filtering)
  - Full hash verification (BLAKE3/SHA-256)
  - Content-defined chunking (shifted/inserted duplicates)
- **Actions**:
  - **Simulate** - Show potential savings
  - **Hardlink** - Create filesystem hardlinks
  - **Move to Recycle Bin** - Safe removal
  - **Delete** - Permanent removal

### 📊 **Disk Space Visualization**
- **Interactive treemap** with squarified layout
- **Hierarchical directory structure** visualization
- **Zoom and pan** navigation
- **Color coding** by file type, size, or owner
- **Tooltips** with detailed file information

### 🧹 **Residue Detection and Cleanup**
- **Orphaned files** detection in program directories
- **Temporary files** cleanup (cache, logs, etc.)
- **Empty directories** removal
- **Registry residue** detection (Windows only)
- **Safe cleanup** with simulation mode

### 🎯 **Perceptual Duplicate Detection**
- **Image pHash** - 32×32 DCT-based perceptual hashing
- **Audio fingerprinting** - Chromatic energy with min-hash
- **LSH indexing** - Fast approximate nearest neighbor search
- **Adjustable thresholds** - Control similarity sensitivity

## 🖥️ **Cross-Platform Support**

### Windows
| Architecture | Status |
|--------------|--------|
| x86 (32-bit) | ✅ Supported |
| x64 (64-bit) | ✅ Supported |

### Linux
| Architecture | Status |
|--------------|--------|
| x86 (32-bit) | ✅ Supported |
| x64 (64-bit) | ✅ Supported |

### macOS
| Architecture | Status |
|--------------|--------|
| x64 (64-bit) | 🚧 Planned |

## 🏗️ **Architecture**

```
DiskSense64/
├── apps/
│   ├── DiskSense.Cli/      # Command-line interface
│   └── DiskSense.Gui/      # Graphical interface
│       ├── components/     # Reusable UI components
│       ├── ui/             # Main UI windows and dialogs
│       └── main.cpp        # Application entry point
├── core/
│   ├── engine/             # I/O scheduler (IOCP/epoll)
│   ├── scan/               # File system scanner
│   ├── index/              # LSM index with mmap
│   ├── model/              # Data structures
│   ├── ops/                # File operations
│   ├── gfx/                # Visualization (GUI only)
│   ├── regcom/             # Registry/COM (Windows only)
│   ├── vss/                # Volume Shadow Copy (Windows only)
│   ├── rules/              # Cleanup heuristics
│   └── usn/                # USN journal (Windows only)
├── libs/
│   ├── chash/             # Cryptographic hashing
│   ├── phash/             # Perceptual hashing
│   ├── audfp/             # Audio fingerprinting
│   ├── peparse/           # PE parser
│   ├── lsh/               # Locality-sensitive hashing
│   └── utils/             # Utility functions
├── platform/
│   ├── fswin/             # Windows file system abstractions
│   └── util/              # Cross-platform utilities
├── tests/
│   ├── unit/              # Unit tests
│   └── perf/              # Performance benchmarks
├── docs/                  # Documentation
├── cmake/                 # CMake configurations
├── scripts/               # Build and utility scripts
└── res/                   # Resources
```

## 🚀 **Installation**

### Windows (Pre-built binaries)

1. Download the latest release from [GitHub Releases](https://github.com/yourusername/disksense64/releases)
2. Extract the archive
3. Run `DiskSense.Cli.exe` or `DiskSense.Gui.exe`

### Linux (Pre-built binaries)

1. Download the latest release from [GitHub Releases](https://github.com/yourusername/disksense64/releases)
2. Extract the archive
3. Run `./DiskSense.Cli` or `./DiskSense.Gui`

## 🔧 **Building from Source**

### Prerequisites

**Windows:**
- Visual Studio 2022 or MinGW-w64
- CMake 3.20+
- Ninja (optional)

**Linux:**
- GCC 9+ or Clang 10+
- CMake 3.20+
- Ninja (optional)

### Building

**Using the build script (recommended):**
```bash
./build_project.sh
```

**Windows (Visual Studio):**
```cmd
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Release
```

**Windows (MinGW-w64):**
```cmd
mkdir build
cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

**Linux:**
```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

**Cross-compilation (Linux to Windows):**
```bash
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/mingw64.cmake -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

## 📖 **Usage**

### Command Line Interface

```bash
# Scan a directory and build index
DiskSense.Cli scan /path/to/directory

# Find duplicates (simulation mode)
DiskSense.Cli dedupe /path/to/directory

# Find duplicates and perform hardlinking
DiskSense.Cli dedupe --action=hardlink /path/to/directory

# Generate treemap visualization (GUI only)
DiskSense.Gui treemap /path/to/directory

# Find similar files
DiskSense.Cli similar /path/to/directory

# Clean residue files
DiskSense.Cli cleanup /path/to/directory
```

### Graphical Interface

Launch `DiskSense.Gui` for the full graphical experience with:
- Interactive treemap visualization
- Real-time disk usage monitoring
- One-click deduplication
- Visual similarity detection

## ⚡ **Performance**

| Operation | Target Performance |
|-----------|-------------------|
| File scanning | ≥ 200-400k files/minute (SSD NVMe) |
| Hashing (BLAKE3) | ≥ 1.5 GB/s |
| Hashing (SHA-256) | ≥ 0.6-1.0 GB/s |
| pHash | ≥ 20k images/minute |
| Treemap | ≤ 16.6 ms per frame (60 FPS) |

## 🔐 **Security & Privacy**

- **Local Processing** - All analysis happens on your machine
- **No Data Transmission** - No files leave your computer
- **Transparency** - Simulation mode shows all actions before execution
- **Revertible** - All operations can be undone
- **Audit Trail** - Detailed logs of all operations

## 🤝 **Contributing**

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

### Development Setup

```bash
git clone https://github.com/yourusername/disksense64.git
cd disksense64
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)
```

## 🧪 **Testing**

Run unit tests:
```bash
cd build
ctest
```

Run performance benchmarks:
```bash
./bin/bench_hash
./bin/bench_io
./bin/bench_phash
```

## 📄 **License**

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 **Acknowledgments**

- [BLAKE3](https://github.com/BLAKE3-team/BLAKE3) - Cryptographic hash function
- [Direct2D](https://docs.microsoft.com/en-us/windows/win32/direct2d/direct2d-portal) - Hardware-accelerated 2D graphics API
- [epoll](https://en.wikipedia.org/wiki/Epoll) - Linux I/O event notification facility
- [I/O Completion Ports](https://docs.microsoft.com/en-us/windows/win32/fileio/i-o-completion-ports) - Windows scalable I/O model

## 🆘 **Support**

For issues, feature requests, or questions:
1. Check the [GitHub Issues](https://github.com/yourusername/disksense64/issues)
2. Create a new issue with detailed information
3. Include your platform, architecture, and error messages

---

*DiskSense64 - Your intelligent disk analysis companion*