# Build Instructions

This document provides detailed instructions for building DiskSense64 on different platforms.

## Prerequisites

### Windows

1. **Visual Studio 2022** or later with C++ development tools
   - Desktop development with C++ workload
   - Windows 11 SDK (10.0.22000.0 or later)
   - CMake tools for C++

2. **Alternative**: MinGW-w64
   - Download from: https://www.mingw-w64.org/
   - Add to PATH environment variable

3. **Git** (for cloning repository)
   - Download from: https://git-scm.com/

### Linux

1. **GCC 9+** or **Clang 10+**
   ```bash
   # Ubuntu/Debian
   sudo apt-get update
   sudo apt-get install build-essential gcc g++ clang
   
   # Fedora/RHEL
   sudo dnf install gcc gcc-c++ clang make
   
   # Arch Linux
   sudo pacman -S gcc clang make
   ```

2. **CMake 3.20+**
   ```bash
   # Ubuntu/Debian
   sudo apt-get install cmake
   
   # Fedora/RHEL
   sudo dnf install cmake
   
   # Arch Linux
   sudo pacman -S cmake
   ```

3. **Ninja** (optional but recommended)
   ```bash
   # Ubuntu/Debian
   sudo apt-get install ninja-build
   
   # Fedora/RHEL
   sudo dnf install ninja-build
   
   # Arch Linux
   sudo pacman -S ninja
   ```

4. **Git**
   ```bash
   # Ubuntu/Debian
   sudo apt-get install git
   
   # Fedora/RHEL
   sudo dnf install git
   
   # Arch Linux
   sudo pacman -S git
   ```

### Cross-Compilation (Linux to Windows)

1. **MinGW-w64 cross-compiler**
   ```bash
   # Ubuntu/Debian
   sudo apt-get install gcc-mingw-w64 mingw-w64-common mingw-w64-i686-dev mingw-w64-x86-64-dev
   
   # Fedora/RHEL
   sudo dnf install mingw64-gcc mingw64-gcc-c++ mingw32-gcc mingw32-gcc-c++
   
   # Arch Linux
   sudo pacman -S mingw-w64-gcc
   ```

## Cloning the Repository

```bash
git clone https://github.com/yourusername/disksense64.git
cd disksense64
```

## Building on Windows

### Method 1: Visual Studio (Recommended)

1. **Generate Solution Files**
   ```cmd
   mkdir build
   cd build
   cmake .. -G "Visual Studio 17 2022" -A x64
   ```

2. **Build Using Visual Studio**
   - Open `DiskSense64.sln` in Visual Studio
   - Select build configuration (Debug/Release)
   - Build solution (Ctrl+Shift+B)

3. **Build Using Command Line**
   ```cmd
   cmake --build . --config Release
   ```

### Method 2: MinGW-w64

1. **Generate Makefiles**
   ```cmd
   mkdir build
   cd build
   cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
   ```

2. **Build**
   ```cmd
   cmake --build .
   ```

### Method 3: Ninja

1. **Generate Ninja Files**
   ```cmd
   mkdir build
   cd build
   cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=Release
   ```

2. **Build**
   ```cmd
   ninja
   ```

## Building on Linux

### Native Build

1. **Generate Build Files**
   ```bash
   mkdir build
   cd build
   cmake .. -DCMAKE_BUILD_TYPE=Release
   ```

2. **Build**
   ```bash
   make -j$(nproc)
   ```

### Ninja Build

1. **Generate Ninja Files**
   ```bash
   mkdir build
   cd build
   cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=Release
   ```

2. **Build**
   ```bash
   ninja
   ```

## Cross-Compilation (Linux to Windows)

1. **Using MinGW-w64 Toolchain**
   ```bash
   mkdir build-win64
   cd build-win64
   cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/mingw64.cmake -DCMAKE_BUILD_TYPE=Release
   make -j$(nproc)
   ```

2. **Custom Toolchain File**
   Create `cmake/mingw64.cmake`:
   ```cmake
   # Sample MinGW-w64 toolchain file
   set(CMAKE_SYSTEM_NAME Windows)
   set(CMAKE_SYSTEM_PROCESSOR x86_64)
   
   set(TOOLCHAIN_PREFIX x86_64-w64-mingw32)
   
   set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}-gcc)
   set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}-g++)
   set(CMAKE_RC_COMPILER ${TOOLCHAIN_PREFIX}-windres)
   
   set(CMAKE_FIND_ROOT_PATH /usr/${TOOLCHAIN_PREFIX})
   
   set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
   set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
   set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
   ```

## Build Configurations

### Debug Build
```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug
```

### Release Build
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release
```

### RelWithDebInfo Build
```bash
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo
```

### MinSizeRel Build
```bash
cmake .. -DCMAKE_BUILD_TYPE=MinSizeRel
```

## Custom Build Options

### Enable/Disable Features
```bash
cmake .. -DENABLE_GUI=ON -DENABLE_TESTS=ON -DENABLE_BENCHMARKS=OFF
```

### Set Installation Prefix
```bash
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
```

### Enable Static Linking
```bash
cmake .. -DSTATIC_LINKING=ON
```

### Enable Advanced Features
```bash
cmake .. -DENABLE_ETW_TRACING=ON -DENABLE_PERFORMANCE_MONITORING=ON
```

## Building Specific Components

### CLI Only
```bash
cmake --build . --target DiskSense.Cli
```

### GUI Only
```bash
cmake --build . --target DiskSense.Gui
```

### Tests
```bash
cmake --build . --target RUN_TESTS
```

### Benchmarks
```bash
cmake --build . --target bench_hash
cmake --build . --target bench_io
cmake --build . --target bench_phash
```

## Installation

### Windows
```cmd
cmake --install . --prefix "C:\Program Files\DiskSense64"
```

### Linux
```bash
sudo cmake --install . --prefix /usr/local
```

## Creating Packages

### Windows Installer (NSIS)
1. Install NSIS: http://nsis.sourceforge.net/
2. ```bash
   cmake .. -DCPACK_GENERATOR=NSIS
   cpack
   ```

### Debian Package
```bash
cmake .. -DCPACK_GENERATOR=DEB
cpack
```

### RPM Package
```bash
cmake .. -DCPACK_GENERATOR=RPM
cpack
```

### ZIP Archive
```bash
cmake .. -DCPACK_GENERATOR=ZIP
cpack
```

## Troubleshooting

### Common Issues

1. **Missing Dependencies**
   ```
   CMake Error: Could not find required package
   ```
   **Solution**: Install missing dependencies using package manager

2. **Compiler Not Found**
   ```
   The C compiler identification is unknown
   ```
   **Solution**: Ensure compiler is installed and in PATH

3. **Insufficient Memory**
   ```
   virtual memory exhausted: Cannot allocate memory
   ```
   **Solution**: Reduce parallel jobs (`make -j4` instead of `make -j$(nproc)`)

4. **Windows SDK Issues**
   ```
   Windows SDK not found
   ```
   **Solution**: Install/reinstall Windows SDK

### Platform-Specific Tips

#### Windows
- Use Visual Studio for best debugging experience
- Enable ETW tracing for performance analysis
- Use Windows Performance Analyzer for profiling

#### Linux
- Use Valgrind for memory leak detection
- Use perf for CPU profiling
- Enable core dumps for crash analysis

#### Cross-Compilation
- Ensure MinGW-w64 libraries are installed
- Test executables in Windows VM or Wine
- Check for Windows-specific API usage

## Performance Optimization

### Compiler Flags
```bash
cmake .. -DCMAKE_CXX_FLAGS="-O3 -march=native -mtune=native"
```

### Link-Time Optimization
```bash
cmake .. -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON
```

### Profile-Guided Optimization
```bash
# Step 1: Instrumentation build
cmake .. -DCMAKE_BUILD_TYPE=Release -DENABLE_PGO=INSTRUMENT

# Step 2: Run training workload
# Step 3: Optimization build
cmake .. -DENABLE_PGO=OPTIMIZE
```

## Continuous Integration

### GitHub Actions
The project includes GitHub Actions workflows in `.github/workflows/`:
- `build.yml`: Builds and tests on multiple platforms
- `release.yml`: Creates releases with packages
- `codeql-analysis.yml`: Security scanning

### Local CI Testing
```bash
# Run all tests
ctest

# Run specific test
ctest -R test_name

# Run with verbose output
ctest -V
```

## Development Workflow

### Setting Up Development Environment

1. **Clone and Build**
   ```bash
   git clone https://github.com/yourusername/disksense64.git
   cd disksense64
   mkdir build && cd build
   cmake .. -DCMAKE_BUILD_TYPE=Debug
   make -j$(nproc)
   ```

2. **Run Tests**
   ```bash
   ctest -V
   ```

3. **Run Benchmarks**
   ```bash
   ./bin/bench_hash
   ./bin/bench_io
   ./bin/bench_phash
   ```

### Code Formatting

The project uses clang-format for consistent code style:
```bash
# Format all source files
find . -name "*.cpp" -o -name "*.h" | xargs clang-format -i

# Check format without modifying files
find . -name "*.cpp" -o -name "*.h" | xargs clang-format --dry-run -Werror
```

### Static Analysis

#### Clang Static Analyzer
```bash
scan-build make
```

#### Cppcheck
```bash
cppcheck --enable=all --inconclusive .
```

#### PVS-Studio (Commercial)
```bash
pvs-studio-analyzer analyze make
plog-converter -a GA:1,2 -t tasklist -o report.tasks PVS-Studio.log
```

## Deployment

### Production Build Checklist

1. [ ] Compile in Release mode with optimizations
2. [ ] Run all tests successfully
3. [ ] Verify performance benchmarks meet targets
4. [ ] Check for memory leaks
5. [ ] Validate cross-platform compatibility
6. [ ] Update version numbers
7. [ ] Generate documentation
8. [ ] Create installation packages
9. [ ] Test installation process
10. [ ] Verify uninstallation process

### Versioning Strategy

The project follows Semantic Versioning (SemVer):
- MAJOR version for incompatible API changes
- MINOR version for backwards-compatible functionality
- PATCH version for backwards-compatible bug fixes

Update version in:
- `CMakeLists.txt`: `project(DiskSense64 VERSION x.y.z)`
- `apps/DiskSense.Cli/main.cpp`: Version string
- `resources/app.manifest`: Version information

---

For questions or issues with building, please check the [GitHub Issues](https://github.com/yourusername/disksense64/issues) or contact the maintainers.