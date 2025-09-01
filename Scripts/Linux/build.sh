#!/bin/bash
set -e

echo "Building Vortex Engine on Linux..."

# Get the script directory and navigate to project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
cd "$PROJECT_ROOT"

echo "Project root: $PROJECT_ROOT"

# Check if we have a Makefile, if not run setup first
if [ ! -f "Makefile" ] && [ ! -f "build.ninja" ]; then
    echo "No build system files found! Running setup first..."
    "$SCRIPT_DIR/setup_complete.sh"
    if [ $? -ne 0 ]; then
        echo "Setup failed!"
        exit 1
    fi
fi

# Determine build system to use
if [ -f "build.ninja" ]; then
    echo "Using Ninja build system..."
    BUILD_CMD="ninja"
    
    # Check if ninja is installed
    if ! command -v ninja &> /dev/null; then
        echo "Ninja not found! Installing ninja..."
        if command -v apt-get &> /dev/null; then
            sudo apt-get update && sudo apt-get install -y ninja-build
        elif command -v dnf &> /dev/null; then
            sudo dnf install -y ninja-build
        elif command -v pacman &> /dev/null; then
            sudo pacman -S --noconfirm ninja
        elif command -v zypper &> /dev/null; then
            sudo zypper install -y ninja
        else
            echo "Please install ninja manually"
            exit 1
        fi
    fi
    
    # Build with ninja
    echo "Building Debug configuration..."
    ninja -C Build/Debug-linux-x86_64
    
    echo "Building Release configuration..."
    ninja -C Build/Release-linux-x86_64
    
elif [ -f "Makefile" ]; then
    echo "Using Make build system..."
    
    # Build with make
    echo "Building Debug configuration..."
    make config=debug -j$(nproc)
    
    echo "Building Release configuration..."
    make config=release -j$(nproc)
    
else
    echo "No supported build system found!"
    exit 1
fi

echo "Build completed successfully!"
echo ""
echo "Debug binaries location: Build/Debug-linux-x86_64/"
echo "Release binaries location: Build/Release-linux-x86_64/"
echo ""
echo "To run the sandbox application:"
echo "  Debug:   ./Build/Debug-linux-x86_64/Sandbox/Sandbox"
echo "  Release: ./Build/Release-linux-x86_64/Sandbox/Sandbox"
