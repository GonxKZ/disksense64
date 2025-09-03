# DiskSense64 Project Structure

This document explains the organized structure of the DiskSense64 project.

## Directory Structure

```
DiskSense64/
├── apps/                    # Application entry points
│   ├── DiskSense.Cli/      # Command-line interface
│   └── DiskSense.Gui/      # Graphical interface
│       ├── components/     # Reusable UI components
│       ├── ui/             # Main UI windows and dialogs
│       └── main.cpp        # Application entry point
├── core/                   # Core libraries and engine components
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
├── libs/                   # Third-party and utility libraries
│   ├── chash/             # Cryptographic hashing
│   ├── phash/             # Perceptual hashing
│   ├── audfp/             # Audio fingerprinting
│   ├── peparse/           # PE parser
│   ├── lsh/               # Locality-sensitive hashing
│   └── utils/             # Utility functions
├── platform/               # Platform-specific code
│   ├── fswin/             # Windows file system abstractions
│   └── util/              # Cross-platform utilities
├── tests/                  # Unit tests and benchmarks
│   ├── unit/              # Unit tests
│   └── perf/              # Performance benchmarks
├── docs/                   # Documentation
├── cmake/                  # CMake configurations
├── scripts/                # Build and utility scripts
├── res/                    # Resources (icons, images, etc.)
├── include/                # Public headers (for external use)
├── src/                    # Source code (alternative organization)
├── build/                  # Build output directory
└── build_project.sh        # Build script
```

## GUI Components

The GUI has been reorganized into modular components:

1. **Main Window** (`apps/DiskSense.Gui/ui/mainwindow.h`): 
   - Contains the main application window with tabbed interface
   - Manages all UI components and core functionality

2. **Custom Components** (`apps/DiskSense.Gui/components/`):
   - `TreemapWidget`: Enhanced treemap visualization with zoom and pan
   - `ResultsDisplay`: Unified results display with progress tracking

3. **UI Libraries** (`apps/DiskSense.Gui/CMakeLists.txt`):
   - Separated UI components into reusable libraries
   - Better organization and dependency management

## Build System

The project uses CMake for cross-platform builds. A helper script `build_project.sh` is provided for easier building.

To build the project:
```bash
./build_project.sh
```

The binaries will be located in `build/bin/` after successful compilation.