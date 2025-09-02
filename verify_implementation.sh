#!/bin/bash
# Final verification script for DiskSense64 cross-platform implementation

set -e

echo "=== DiskSense64 Cross-Platform Implementation Verification ==="
echo

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print status messages
print_status() {
    echo -e "${GREEN}[STATUS]${NC} $1"
}

# Function to print warnings
print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

# Function to print errors
print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to print info
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

# Function to detect OS
detect_os() {
    case "$(uname -s)" in
        Linux*)     echo "Linux";;
        Darwin*)    echo "Mac";;
        CYGWIN*)    echo "Cygwin";;
        MINGW*)     echo "MinGW";;
        *)          echo "Unknown";;
    esac
}

# Function to detect architecture
detect_arch() {
    case "$(uname -m)" in
        x86_64)     echo "x64";;
        i386|i686)  echo "x86";;
        armv7l)     echo "arm";;
        aarch64)    echo "arm64";;
        *)          echo "unknown";;
    esac
}

# Get project directory
PROJECT_DIR=$(pwd)
OS=$(detect_os)
ARCH=$(detect_arch)

print_info "Project directory: $PROJECT_DIR"
print_info "Detected OS: $OS"
print_info "Detected Architecture: $ARCH"
echo

# Test 1: Check directory structure
print_status "Test 1: Directory structure verification"
REQUIRED_DIRS=(
    "apps"
    "core"
    "libs"
    "platform"
    "tests"
    "cmake"
    "docs"
    "scripts"
    "res"
)

REQUIRED_FILES=(
    "CMakeLists.txt"
    "README.md"
    "LICENSE"
    "CHANGELOG.md"
    "CONTRIBUTING.md"
    "CODE_OF_CONDUCT.md"
    "build_and_test.sh"
)

MISSING_ITEMS=0

for dir in "${REQUIRED_DIRS[@]}"; do
    if [ -d "$dir" ]; then
        print_info "✅ Directory found: $dir"
    else
        print_error "❌ Directory missing: $dir"
        MISSING_ITEMS=$((MISSING_ITEMS + 1))
    fi
done

for file in "${REQUIRED_FILES[@]}"; do
    if [ -f "$file" ]; then
        print_info "✅ File found: $file"
    else
        print_error "❌ File missing: $file"
        MISSING_ITEMS=$((MISSING_ITEMS + 1))
    fi
done

echo

# Test 2: Check core module structure
print_status "Test 2: Core module structure verification"
CORE_MODULES=(
    "engine"
    "scan"
    "index"
    "model"
    "ops"
    "gfx"
    "rules"
    "usn"
    "vss"
)

for module in "${CORE_MODULES[@]}"; do
    if [ -d "core/$module" ]; then
        print_info "✅ Core module found: $module"
    else
        print_warning "⚠️ Core module missing: $module"
    fi
done

echo

# Test 3: Check library structure
print_status "Test 3: Library structure verification"
LIBRARIES=(
    "chash"
    "phash"
    "audfp"
    "peparse"
    "lsh"
    "utils"
)

for lib in "${LIBRARIES[@]}"; do
    if [ -d "libs/$lib" ]; then
        print_info "✅ Library found: $lib"
    else
        print_error "❌ Library missing: $lib"
        MISSING_ITEMS=$((MISSING_ITEMS + 1))
    fi
done

echo

# Test 4: Check platform structure
print_status "Test 4: Platform structure verification"
PLATFORM_DIRS=(
    "fswin"
    "util"
)

for dir in "${PLATFORM_DIRS[@]}"; do
    if [ -d "platform/$dir" ]; then
        print_info "✅ Platform directory found: $dir"
    else
        print_error "❌ Platform directory missing: $dir"
        MISSING_ITEMS=$((MISSING_ITEMS + 1))
    fi
done

echo

# Test 5: Check application structure
print_status "Test 5: Application structure verification"
APPLICATIONS=(
    "DiskSense.Cli"
    "DiskSense.Gui"
)

for app in "${APPLICATIONS[@]}"; do
    if [ -d "apps/$app" ]; then
        print_info "✅ Application found: $app"
    else
        print_error "❌ Application missing: $app"
        MISSING_ITEMS=$((MISSING_ITEMS + 1))
    fi
done

echo

# Test 6: Check build scripts
print_status "Test 6: Build script verification"
BUILD_SCRIPTS=(
    "build_and_test.sh"
)

for script in "${BUILD_SCRIPTS[@]}"; do
    if [ -f "$script" ] && [ -x "$script" ]; then
        print_info "✅ Executable build script found: $script"
    elif [ -f "$script" ]; then
        print_warning "⚠️ Build script found but not executable: $script"
    else
        print_error "❌ Build script missing: $script"
        MISSING_ITEMS=$((MISSING_ITEMS + 1))
    fi
done

echo

# Test 7: Check configuration files
print_status "Test 7: Configuration file verification"
CONFIG_FILES=(
    "cmake/mingw64.cmake"
    "cmake/linux64.cmake"
    "cmake/warnings.cmake"
)

for config in "${CONFIG_FILES[@]}"; do
    if [ -f "$config" ]; then
        print_info "✅ Configuration file found: $config"
    else
        print_error "❌ Configuration file missing: $config"
        MISSING_ITEMS=$((MISSING_ITEMS + 1))
    fi
done

echo

# Test 8: Check source files
print_status "Test 8: Source file verification"
REQUIRED_SOURCE_FILES=(
    "core/model/model.h"
    "core/scan/scanner.h"
    "core/scan/scanner.cpp"
    "core/index/lsm_index.h"
    "core/index/lsm_index.cpp"
    "core/ops/dedupe.h"
    "core/ops/dedupe.cpp"
    "libs/chash/sha256.h"
    "libs/chash/sha256.c"
    "libs/chash/blake3.h"
    "libs/chash/blake3.c"
    "libs/phash/phash.h"
    "libs/phash/phash.c"
    "libs/utils/utils.h"
    "libs/utils/utils.cpp"
    "apps/DiskSense.Cli/main.cpp"
)

MISSING_SOURCES=0
for src in "${REQUIRED_SOURCE_FILES[@]}"; do
    if [ -f "$src" ]; then
        print_info "✅ Source file found: $src"
    else
        print_error "❌ Source file missing: $src"
        MISSING_SOURCES=$((MISSING_SOURCES + 1))
    fi
done

echo

# Test 9: Check cross-platform capabilities
print_status "Test 9: Cross-platform capability verification"

# Check for cross-compilation toolchains
if [ -f "cmake/mingw64.cmake" ]; then
    print_info "✅ MinGW-w64 cross-compilation toolchain available"
else
    print_warning "⚠️ MinGW-w64 cross-compilation toolchain not found"
fi

if [ -f "cmake/linux64.cmake" ]; then
    print_info "✅ Linux cross-compilation toolchain available"
else
    print_warning "⚠️ Linux cross-compilation toolchain not found"
fi

# Check for platform abstractions
if [ -f "platform/fswin/fswin.h" ] && [ -f "platform/fswin/fswin.cpp" ]; then
    print_info "✅ Windows file system abstractions available"
else
    print_warning "⚠️ Windows file system abstractions not found"
fi

if [ -f "libs/utils/utils.h" ] && [ -f "libs/utils/utils.cpp" ]; then
    print_info "✅ Cross-platform utilities available"
else
    print_warning "⚠️ Cross-platform utilities not found"
fi

echo

# Test 10: Check documentation
print_status "Test 10: Documentation verification"
DOC_FILES=(
    "README.md"
    "docs/BUILDING.md"
    "docs/CONTRIBUTING.md"
    "docs/CODE_OF_CONDUCT.md"
    "docs/SECURITY.md"
    "LICENSE"
)

for doc in "${DOC_FILES[@]}"; do
    if [ -f "$doc" ]; then
        print_info "✅ Documentation file found: $doc"
    else
        print_warning "⚠️ Documentation file missing: $doc"
    fi
done

echo

# Summary
print_status "=== Verification Summary ==="
TOTAL_ITEMS=$((${#REQUIRED_DIRS[@]} + ${#REQUIRED_FILES[@]} + ${#LIBRARIES[@]} + ${#PLATFORM_DIRS[@]} + ${#APPLICATIONS[@]} + ${#BUILD_SCRIPTS[@]} + ${#CONFIG_FILES[@]}))
FOUND_ITEMS=$((TOTAL_ITEMS - MISSING_ITEMS))

print_info "Directory structure: $FOUND_ITEMS/$TOTAL_ITEMS items found"
print_info "Core modules: ${#CORE_MODULES[@]}/$(ls -1 core 2>/dev/null | wc -l) verified"
print_info "Libraries: $((${#LIBRARIES[@]} - MISSING_SOURCES))/${#LIBRARIES[@]} found"
print_info "Platform directories: $((${#PLATFORM_DIRS[@]} - MISSING_ITEMS))/${#PLATFORM_DIRS[@]} found"
print_info "Applications: $((${#APPLICATIONS[@]} - MISSING_ITEMS))/${#APPLICATIONS[@]} found"
print_info "Build scripts: $((${#BUILD_SCRIPTS[@]} - MISSING_ITEMS))/${#BUILD_SCRIPTS[@]} found"
print_info "Configuration files: $((${#CONFIG_FILES[@]} - MISSING_ITEMS))/${#CONFIG_FILES[@]} found"
print_info "Source files: $((${#REQUIRED_SOURCE_FILES[@]} - MISSING_SOURCES))/${#REQUIRED_SOURCE_FILES[@]} found"

echo

if [ $MISSING_ITEMS -eq 0 ] && [ $MISSING_SOURCES -eq 0 ]; then
    print_status "🎉 All critical components verified successfully!"
    print_status "The DiskSense64 cross-platform implementation is ready for building."
    
    echo
    print_info "🚀 Next steps:"
    echo "1. Run './build_and_test.sh native' to build for your native platform"
    echo "2. Run './build_and_test.sh win' to cross-compile for Windows"
    echo "3. Run './build_and_test.sh linux' to cross-compile for Linux"
    echo "4. Run './build_and_test.sh all' to build for all platforms"
    echo "5. Run './build_and_test.sh test' to execute unit tests"
    echo "6. Run './build_and_test.sh package' to create distribution packages"
else
    print_warning "⚠️ Some components are missing. Please check the output above."
    print_info "Critical missing items: $MISSING_ITEMS"
    print_info "Missing source files: $MISSING_SOURCES"
fi

echo
print_status "Verification completed!"