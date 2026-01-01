#!/bin/bash
set -e

# Build and run the si-units example

echo "=== Building si-units example ==="

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

# Run the example
echo ""
echo "=== Running example ==="
./my-project-using-si-units

echo ""
echo "=== Example completed successfully ==="
