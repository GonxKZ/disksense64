#!/bin/bash
# Final verification script for DiskSense64 cross-platform implementation

echo "=== DiskSense64 Cross-Platform Verification ==="
echo

# Check if we're on WSL
if grep -q microsoft /proc/version; then
    ENV="WSL"
else
    ENV="Linux"
fi

echo "Environment: $ENV"
echo

# Test 1: Check directory structure
echo "Test 1: Directory structure verification"
REQUIRED_DIRS=(
    "apps"
    "core"
    "libs"
    "platform"
    "tests"
    "cmake"
)

REQUIRED_FILES=(
    "CMakeLists.txt"
    "README.md"
    "LICENSE"
    "CHANGELOG.md"
    "CONTRIBUTING.md"
    "CODE_OF_CONDUCT.md"
)

MISSING_ITEMS=0

for dir in "${REQUIRED_DIRS[@]}"; do
    if [ ! -d "$dir" ]; then
        echo "❌ Missing directory: $dir"
        MISSING_ITEMS=$((MISSING_ITEMS + 1))
    else
        echo "✅ Found directory: $dir"
    fi
done

for file in "${REQUIRED_FILES[@]}"; do
    if [ ! -f "$file" ]; then
        echo "❌ Missing file: $file"
        MISSING_ITEMS=$((MISSING_ITEMS + 1))
    else
        echo "✅ Found file: $file"
    fi
done

echo
echo "Directory structure check: $(( ${#REQUIRED_DIRS[@]} + ${#REQUIRED_FILES[@]} - MISSING_ITEMS ))/${#REQUIRED_DIRS[@]} directories and files found"
echo

# Test 2: Check core module structure
echo "Test 2: Core module structure verification"
CORE_MODULES=(
    "engine"
    "scan"
    "index"
    "model"
    "ops"
    "gfx"
    "rules"
)

for module in "${CORE_MODULES[@]}"; do
    if [ -d "core/$module" ]; then
        echo "✅ Core module found: $module"
    else
        echo "⚠️  Core module missing: $module (may be optional)"
    fi
done

echo

# Test 3: Check library structure
echo "Test 3: Library structure verification"
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
        echo "✅ Library found: $lib"
    else
        echo "❌ Library missing: $lib"
    fi
done

echo

# Test 4: Check CMake configuration
echo "Test 4: CMake configuration verification"
if [ -f "cmake/mingw64.cmake" ]; then
    echo "✅ MinGW-w64 toolchain configuration found"
else
    echo "❌ MinGW-w64 toolchain configuration missing"
fi

if [ -f "cmake/warnings.cmake" ]; then
    echo "✅ Compiler warnings configuration found"
else
    echo "❌ Compiler warnings configuration missing"
fi

echo

# Test 5: Check build scripts
echo "Test 5: Build script verification"
BUILD_SCRIPTS=(
    "build.sh"
    "build_and_test.sh"
)

for script in "${BUILD_SCRIPTS[@]}"; do
    if [ -f "$script" ] && [ -x "$script" ]; then
        echo "✅ Executable build script found: $script"
    elif [ -f "$script" ]; then
        echo "⚠️  Build script found but not executable: $script"
    else
        echo "❌ Build script missing: $script"
    fi
done

echo

# Test 6: Check source files exist
echo "Test 6: Source file verification"
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
        echo "✅ Source file found: $src"
    else
        echo "❌ Source file missing: $src"
        MISSING_SOURCES=$((MISSING_SOURCES + 1))
    fi
done

echo

# Test 7: Check test files
echo "Test 7: Test file verification"
TEST_FILES=(
    "tests/unit/test_sha256.cpp"
    "tests/unit/test_utils.cpp"
)

for test in "${TEST_FILES[@]}"; do
    if [ -f "$test" ]; then
        echo "✅ Test file found: $test"
    else
        echo "❌ Test file missing: $test"
    fi
done

echo

# Summary
echo "=== Verification Summary ==="
echo "Environment: $ENV"
echo "Directory structure: $(( ${#REQUIRED_DIRS[@]} + ${#REQUIRED_FILES[@]} - MISSING_ITEMS ))/${#REQUIRED_DIRS[@]} + ${#REQUIRED_FILES[@]} items found"
echo "Core modules: ${#CORE_MODULES[@]}/$(ls -1 core 2>/dev/null | wc -l) verified"
echo "Libraries: $(( ${#LIBRARIES[@]} - MISSING_SOURCES ))/${#LIBRARIES[@]} found"
echo "Source files: $(( ${#REQUIRED_SOURCE_FILES[@]} - MISSING_SOURCES ))/${#REQUIRED_SOURCE_FILES[@]} found"
echo "Build scripts: ${#BUILD_SCRIPTS[@]}/$(ls -1 *.sh 2>/dev/null | wc -l) verified"
echo

if [ $MISSING_ITEMS -eq 0 ] && [ $MISSING_SOURCES -eq 0 ]; then
    echo "🎉 All critical components verified successfully!"
    echo "The DiskSense64 cross-platform implementation is ready for building."
else
    echo "⚠️  Some components are missing. Please check the output above."
fi

echo
echo "Next steps:"
echo "1. Run './build.sh all' to build for all platforms"
echo "2. Run './build_and_test.sh test' to execute unit tests"
echo "3. Run './build_and_test.sh package' to create distribution packages"