#!/bin/bash
# Test script for DiskSense64 CLI

echo "=== Testing DiskSense64 CLI ==="

# Create a test directory
TEST_DIR="/tmp/disksense64_test"
mkdir -p "$TEST_DIR"

# Create some test files
echo "Creating test files..."
echo "This is test file 1" > "$TEST_DIR/file1.txt"
echo "This is test file 2" > "$TEST_DIR/file2.txt"
echo "This is test file 3" > "$TEST_DIR/file3.txt"
echo "This is test file 1" > "$TEST_DIR/file4.txt"  # Duplicate of file1
echo "This is test file 2" > "$TEST_DIR/file5.txt"  # Duplicate of file2

# Run the CLI application
echo "Running DiskSense64 CLI on test directory..."
./build-linux/DiskSense.Cli scan "$TEST_DIR"

echo "Test completed!"

# Clean up
rm -rf "$TEST_DIR"