# DiskSense64 - Cross-Platform Disk Analysis Suite

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen)](https://github.com/GonxKZ/disksense64)
[![License](https://img.shields.io/badge/license-MIT-blue)](https://opensource.org/licenses/MIT)
[![Platforms](https://img.shields.io/badge/platforms-Windows%20%7C%20Linux-lightgrey)](https://github.com/GonxKZ/disksense64)

A comprehensive, cross-platform disk analysis suite that provides four core capabilities:
1. **Exact File Deduplication** with hardlinks
2. **Disk Space Visualization** with treemap charts
3. **Residue Detection and Cleanup** of orphaned files
4. **Perceptual Duplicate Detection** for images and audio

## 🌟 Features

- 🖥️ **Cross-Platform**: Runs on Windows (32/64-bit) and Linux (32/64-bit)
- ⚡ **High Performance**: Optimized I/O with IOCP (Windows) and epoll (Linux)
- 🔒 **Safe Operations**: Simulation mode, snapshots, and revertible actions
- 📊 **Visual Analytics**: Interactive treemap visualization
- 🧠 **Smart Deduplication**: Multi-stage filtering for efficiency
- 🎯 **Perceptual Similarity**: Find similar images/audio with pHash/min-hash
- 🧹 **Residue Cleanup**: Detect and remove orphaned files and directories
- 🛠️ **No External Dependencies**: Pure C/C++ implementation

## 🚀 Quick Start

### Windows (Pre-built binaries)

1. Download the latest release from [GitHub Releases](https://github.com/GonxKZ/disksense64/releases)
2. Extract the archive
3. Run `DiskSense.Cli.exe` or `DiskSense.Gui.exe`

### Linux (Pre-built binaries)

1. Download the latest release from [GitHub Releases](https://github.com/GonxKZ/disksense64/releases)
2. Extract the archive
3. Run `./DiskSense.Cli` or `./DiskSense.Gui`

## 🏗️ Building from Source

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

## 📖 Usage

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

## 🧰 Core Features

### 1. Exact File Deduplication

Multi-stage filtering for maximum efficiency:
1. **Size-based filtering** - O(1) elimination of unique-sized files
2. **Head/Tail signatures** - 32KB fingerprint for fast candidate filtering
3. **Full hash verification** - BLAKE3/SHA-256 for final verification
4. **Content-defined chunking** - Detects shifted/inserted duplicates

**Actions:**
- **Simulate** - Show potential savings without making changes
- **Hardlink** - Create filesystem hardlinks (space savings)
- **Move to Recycle Bin** - Move duplicates to system trash
- **Delete** - Permanently remove duplicates

### 2. Disk Space Visualization

Interactive treemap with:
- **Squarified algorithm** - Minimizes rectangle aspect ratios
- **Hierarchical layout** - Directory structure visualization
- **Zoom and pan** - Navigate large directory trees
- **Color coding** - By file type, size, or owner
- **Tooltips** - Detailed file information on hover

### 3. Residue Detection

Smart cleanup of:
- **Orphaned files** - Files in program directories with no references
- **Temporary files** - Old cache and temp files
- **Log files** - Large log files that can be truncated
- **Duplicate directories** - Empty or redundant directories

### 4. Perceptual Similarity

Find visually or audibly similar files:
- **Image pHash** - 32×32 DCT-based perceptual hashing
- **Audio fingerprinting** - Chromatic energy with min-hash
- **LSH indexing** - Fast approximate nearest neighbor search
- **Adjustable thresholds** - Control similarity sensitivity

## ⚙️ Performance

| Operation | Target Performance |
|-----------|-------------------|
| File scanning | ≥ 200-400k files/minute (SSD NVMe) |
| Hashing (BLAKE3) | ≥ 1.5 GB/s |
| Hashing (SHA-256) | ≥ 0.6-1.0 GB/s |
| pHash | ≥ 20k images/minute |
| Treemap | ≤ 16.6 ms per frame (60 FPS) |

## 🔧 Configuration

Create a `DiskSense64.config` file in the application directory:

```ini
[General]
BufferSize=1048576
MaxConcurrentOps=8
ExcludePaths=/tmp;/var/log

[Deduplication]
MinFileSize=1024
ComputeFullHash=false
DefaultAction=simulate

[Treemap]
ColorMode=size
ShowFileNames=true

[Similarity]
ImageThreshold=5
AudioThreshold=10
```

## 🔐 Security & Privacy

- 🔐 **Local Processing** - All analysis happens on your machine
- 🛡️ **No Data Transmission** - No files leave your computer
- 🧾 **Transparency** - Simulation mode shows all actions before execution
- 🔁 **Revertible** - All operations can be undone
- 📜 **Audit Trail** - Detailed logs of all operations

## 👥 Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

### Development Setup

```bash
git clone https://github.com/GonxKZ/disksense64.git
cd disksense64
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)
```

## 🧪 Testing

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

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- [BLAKE3](https://github.com/BLAKE3-team/BLAKE3) - Cryptographic hash function
- [Direct2D](https://docs.microsoft.com/en-us/windows/win32/direct2d/direct2d-portal) - Hardware-accelerated 2D graphics API
- [epoll](https://en.wikipedia.org/wiki/Epoll) - Linux I/O event notification facility
- [I/O Completion Ports](https://docs.microsoft.com/en-us/windows/win32/fileio/i-o-completion-ports) - Windows scalable I/O model

## 🆘 Support

For issues, feature requests, or questions:
1. Check the [GitHub Issues](https://github.com/GonxKZ/disksense64/issues)
2. Create a new issue with detailed information
3. Include your platform, architecture, and error messages

---

*DiskSense64 - Your intelligent disk analysis companion*

## 📈 Project Status

✅ **Complete Implementation**: All core features implemented and tested  
✅ **Cross-Platform Support**: Windows (32/64-bit) and Linux (32/64-bit)  
✅ **High Performance**: Optimized for SSD/HDD performance  
✅ **Production Ready**: Ready for real-world use  

## 🎯 Future Roadmap

- 🍎 **macOS Support**: Add support for macOS platforms
- 🌐 **Web Interface**: Browser-based visualization dashboard
- ☁️ **Cloud Integration**: Sync with cloud storage services
- 🤖 **AI Enhancement**: Machine learning for smarter deduplication
- 📱 **Mobile App**: Mobile companion for remote monitoring

---