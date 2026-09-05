#!/bin/bash
set -e

# Build script for si-units library
# Usage:
#   ./run.sh        - Build and run tests
#   ./run.sh clean  - Remove all build directories

# Get the script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Function to clean build directories
clean_builds() {
    echo "=== Cleaning si-units build directories ==="

    cd "$SCRIPT_DIR"

    # Remove main build directory
    if [ -d "build" ]; then
        echo "Removing build/"
        rm -rf build
    fi

    # Remove example build directory
    if [ -d "example/build" ]; then
        echo "Removing example/build/"
        rm -rf example/build
    fi

    echo "=== Clean completed ==="
}

# Check for clean command
if [ "$1" = "clean" ]; then
    clean_builds
    exit 0
fi

# Default behavior: build and test
echo "=== Building si-units library ==="

cd "$SCRIPT_DIR"

# Create build directory
BUILD_DIR="build"
if [ -d "$BUILD_DIR" ]; then
    echo "Cleaning existing build directory..."
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake
echo "Configuring with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
echo "Building..."
cmake --build . --config Release

# Run tests
echo ""
echo "=== Running unit tests ==="
ctest --output-on-failure --verbose

echo ""
echo "=== Build and tests completed successfully ==="
