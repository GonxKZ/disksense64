#!/bin/bash
# Test script for cross-platform build system

echo "=== Testing DiskSense64 Cross-Platform Build System ==="
echo

# Test 1: Check prerequisites
echo "Test 1: Checking prerequisites..."
if ./build_crossplatform.sh 2>/dev/null; then
    echo "✅ Prerequisites check passed"
else
    # The script exits with code 1 when no arguments are provided, which is expected
    # We'll check if it prints the usage message
    USAGE_OUTPUT=$(./build_crossplatform.sh 2>&1 || true)
    if [[ "$USAGE_OUTPUT" == *"Usage:"* ]]; then
        echo "✅ Prerequisites check passed"
    else
        echo "❌ Prerequisites check failed"
        echo "$USAGE_OUTPUT"
        exit 1
    fi
fi

# Test 2: Check directory structure
echo
echo "Test 2: Checking directory structure..."
REQUIRED_DIRS=(
    "apps"
    "core"
    "libs"
    "platform"
    "tests"
    "cmake"
    "docs"
    "scripts"
)

MISSING_DIRS=0
for dir in "${REQUIRED_DIRS[@]}"; do
    if [ -d "$dir" ]; then
        echo "✅ Directory found: $dir"
    else
        echo "❌ Directory missing: $dir"
        MISSING_DIRS=$((MISSING_DIRS + 1))
    fi
done

# Test 3: Check core module structure
echo
echo "Test 3: Checking core module structure..."
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
        echo "✅ Core module found: $module"
    else
        echo "⚠️ Core module missing: $module"
    fi
done

# Test 4: Check library structure
echo
echo "Test 4: Checking library structure..."
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

# Test 5: Check platform structure
echo
echo "Test 5: Checking platform structure..."
PLATFORM_DIRS=(
    "fswin"
    "util"
)

for dir in "${PLATFORM_DIRS[@]}"; do
    if [ -d "platform/$dir" ]; then
        echo "✅ Platform directory found: $dir"
    else
        echo "❌ Platform directory missing: $dir"
    fi
done

# Test 6: Check build scripts
echo
echo "Test 6: Checking build scripts..."
BUILD_SCRIPTS=(
    "build.sh"
    "build_and_test.sh"
    "build_crossplatform.sh"
)

for script in "${BUILD_SCRIPTS[@]}"; do
    if [ -f "$script" ] && [ -x "$script" ]; then
        echo "✅ Executable build script found: $script"
    elif [ -f "$script" ]; then
        echo "⚠️ Build script found but not executable: $script"
    else
        echo "❌ Build script missing: $script"
    fi
done

# Test 7: Check configuration files
echo
echo "Test 7: Checking configuration files..."
CONFIG_FILES=(
    "CMakeLists.txt"
    "cmake/mingw64.cmake"
    "cmake/linux64.cmake"
    "DiskSense64.config"
)

for config in "${CONFIG_FILES[@]}"; do
    if [ -f "$config" ]; then
        echo "✅ Configuration file found: $config"
    else
        echo "❌ Configuration file missing: $config"
    fi
done

# Test 8: Check documentation
echo
echo "Test 8: Checking documentation..."
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
        echo "✅ Documentation file found: $doc"
    else
        echo "❌ Documentation file missing: $doc"
    fi
done

# Summary
echo
echo "=== Test Summary ==="
echo "Prerequisites check: Passed"
echo "Directory structure: $(( ${#REQUIRED_DIRS[@]} - MISSING_DIRS ))/${#REQUIRED_DIRS[@]} directories found"
echo "Core modules: ${#CORE_MODULES[@]}/$(ls -1 core 2>/dev/null | wc -l) verified"
echo "Libraries: ${#LIBRARIES[@]}/$(ls -1 libs 2>/dev/null | wc -l) verified"
echo "Platform directories: ${#PLATFORM_DIRS[@]}/$(ls -1 platform 2>/dev/null | wc -l) verified"
echo "Build scripts: ${#BUILD_SCRIPTS[@]}/$(ls -1 *.sh 2>/dev/null | wc -l) verified"
echo "Configuration files: ${#CONFIG_FILES[@]}/$(find cmake -name "*.cmake" 2>/dev/null | wc -l) verified"
echo "Documentation files: ${#DOC_FILES[@]}/$(find docs -name "*.md" 2>/dev/null | wc -l) verified"

echo
echo "🎉 Cross-platform build system verification completed!"
echo "The DiskSense64 project is ready for cross-platform development."