# DiskSense64 Developer Documentation

## Table of Contents
1. [Project Overview](#project-overview)
2. [Architecture](#architecture)
3. [Core Components](#core-components)
4. [UI Components](#ui-components)
5. [API Reference](#api-reference)
6. [Testing](#testing)
7. [Performance](#performance)
8. [Security](#security)
9. [Network](#network)
10. [Extensibility](#extensibility)

## Project Overview

DiskSense64 is a comprehensive, cross-platform disk analysis suite that provides four core capabilities:
- Exact File Deduplication with hardlinks
- Disk Space Visualization with treemap charts
- Residue Detection and Cleanup of orphaned files
- Perceptual Duplicate Detection for images and audio

## Architecture

The project follows a modular architecture with clear separation of concerns:

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

## Core Components

### File System Scanner
The file system scanner is responsible for traversing directories and collecting file metadata.

Key features:
- Multi-threaded scanning for performance
- Support for large file systems
- Filtering capabilities
- Progress reporting

### LSM Index
The Log-Structured Merge-tree index provides fast key-value storage for file metadata.

Key features:
- Memory-mapped files for performance
- Efficient range queries
- Persistent storage
- Thread-safe operations

### Visualization Engine
The visualization engine generates interactive treemaps and other visualizations.

Key features:
- Squarified treemap layout algorithm
- Hardware-accelerated rendering
- Zoom and pan interactions
- Color coding by file attributes

### Cache System
The cache system provides in-memory and persistent caching for improved performance.

Key features:
- LRU eviction policy
- SQLite backend for persistence
- Automatic cleanup of stale entries
- Memory usage monitoring

### Database Layer
The database layer provides persistent storage for scan results and analysis data.

Key features:
- SQLite backend
- ACID transactions
- Schema versioning
- Query optimization

## UI Components

### Main Window
The main window serves as the primary application interface.

Key features:
- Tab-based navigation
- Menu and toolbar
- Status bar with progress indicator
- Dockable panels

### Dashboard Tab
The dashboard provides an overview of system information and quick statistics.

Key features:
- System information panel
- Quick statistics summary
- Recent scan history
- Quick access buttons

### Settings Dialog
The settings dialog allows users to configure application preferences.

Key features:
- User preferences (theme, language, default paths)
- Performance settings (thread count, memory usage limits)
- Visualization options (color schemes, display preferences)
- Scan exclusions and filters

### File Explorer
The file explorer provides a tree view of the file system.

Key features:
- Tree view of the file system
- Ability to select multiple directories/files
- File properties panel
- Advanced filtering options

### Visualization Widget
The visualization widget provides interactive visualizations.

Key features:
- 2D and 3D treemaps
- Pie charts and bar graphs
- Interactive filtering
- Zoom animations and transitions

## API Reference

### Core Classes

#### Scanner
```cpp
class Scanner {
public:
    struct ScanOptions {
        bool computeHeadTail;
        bool computeFullHash;
        quint64 minFileSize;
        quint64 maxFileSize;
        QStringList includeExtensions;
    };
    
    void scanVolume(const std::string& path, const ScanOptions& options, 
                   std::function<void(const ScanEvent&)> callback);
    void cancel();
};
```

#### LSMIndex
```cpp
class LSMIndex {
public:
    void put(const FileEntry& entry);
    std::vector<FileEntry> getByVolume(int volumeId);
    void flush();
};
```

#### TreemapLayout
```cpp
class TreemapLayout {
public:
    static std::unique_ptr<TreemapNode> createTreemap(
        const std::vector<FileEntry>& files, 
        const Rect& bounds);
};
```

## Testing

### Unit Tests
Unit tests are located in the `tests/unit/` directory and use the Qt Test framework.

To run unit tests:
```bash
cd build
make run_tests
```

### Performance Benchmarks
Performance benchmarks are located in the `tests/perf/` directory.

To run benchmarks:
```bash
cd build
make run_benchmarks
```

### Test Coverage
Test coverage can be generated using lcov:

```bash
cd build
cmake .. -DENABLE_COVERAGE=ON
make coverage
```

## Performance

### Optimization Techniques
- Memory-mapped files for index storage
- Multi-threaded scanning
- Caching of frequently accessed data
- Efficient data structures (LSM trees)
- Hardware-accelerated rendering

### Performance Targets
- File scanning: ≥ 200-400k files/minute (SSD NVMe)
- Hashing (BLAKE3): ≥ 1.5 GB/s
- Hashing (SHA-256): ≥ 0.6-1.0 GB/s
- pHash: ≥ 20k images/minute
- Treemap: ≤ 16.6 ms per frame (60 FPS)

## Security

### Security Features
- Local processing (no data transmission)
- Simulation mode for previewing actions
- Audit trail of all operations
- File permission analysis
- Malware scanning integration
- Encryption detection
- Sensitive file identification

### Security Best Practices
- All analysis happens on the local machine
- No files leave the computer
- Transparency with simulation mode
- Revertible operations
- Detailed logs of all operations

## Network

### Network Features
- Remote scanning capabilities
- Network drive support
- Cloud storage integration
- Bandwidth monitoring
- Authentication and encryption

### Supported Protocols
- SMB/CIFS for Windows shares
- NFS for Unix-like systems
- WebDAV for web-based storage
- Cloud APIs (Dropbox, Google Drive, OneDrive)

## Extensibility

### Plugin Architecture
DiskSense64 supports a plugin architecture for extending functionality.

### API
The application provides a comprehensive API for integration with other tools.

### Custom Rules
Users can define custom rules for cleanup and analysis operations.

### Scripting Interface
The application supports scripting for automation of complex operations.