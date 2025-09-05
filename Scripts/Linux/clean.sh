#!/bin/bash

echo "Cleaning Vortex Engine on Linux..."

# Get the script directory and navigate to project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
cd "$PROJECT_ROOT"

echo "Project root: $PROJECT_ROOT"

echo "Removing build files..."
if [ -d "Build" ]; then
    rm -rf "Build"
    echo "Removed Build directory"
fi

echo "Removing build system files..."
if [ -f "Makefile" ]; then
    rm -f "Makefile"
    echo "Removed Makefile"
fi

if [ -f "build.ninja" ]; then
    rm -f "build.ninja"
    echo "Removed build.ninja"
fi

# Remove other build artifacts
rm -f *.make
rm -f *.ninja
rm -f .ninja_deps
rm -f .ninja_log

# Remove any generated project files
rm -f compile_commands.json

echo "Clean completed!"
echo "Run './Scripts/Linux/setup_complete.sh' to regenerate build files."