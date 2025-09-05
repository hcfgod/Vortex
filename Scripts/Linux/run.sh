#!/bin/bash

# Get the script directory and navigate to project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
cd "$PROJECT_ROOT"

echo "Vortex Engine - Application Runner"
echo "=================================="

# Function to show usage
show_usage() {
    echo "Usage: $0 [application] [configuration]"
    echo ""
    echo "Applications:"
    echo "  sandbox    - Run the Sandbox application (default)"
    echo "  engine     - Run the Engine tests (if available)"
    echo ""
    echo "Configurations:"
    echo "  debug      - Run Debug build (default)"
    echo "  release    - Run Release build"
    echo ""
    echo "Examples:"
    echo "  $0                    # Run Sandbox in Debug mode"
    echo "  $0 sandbox debug     # Run Sandbox in Debug mode"
    echo "  $0 sandbox release   # Run Sandbox in Release mode"
}

# Parse arguments
APP="sandbox"
CONFIG="debug"

if [ $# -ge 1 ]; then
    case $1 in
        sandbox|engine)
            APP=$1
            ;;
        debug|release)
            CONFIG=$1
            ;;
        -h|--help|help)
            show_usage
            exit 0
            ;;
        *)
            echo "Unknown application: $1"
            show_usage
            exit 1
            ;;
    esac
fi

if [ $# -ge 2 ]; then
    case $2 in
        debug|release)
            CONFIG=$2
            ;;
        *)
            echo "Unknown configuration: $2"
            show_usage
            exit 1
            ;;
    esac
fi

# Convert to proper case for directory names
if [ "$CONFIG" = "debug" ]; then
    BUILD_CONFIG="Debug"
elif [ "$CONFIG" = "release" ]; then
    BUILD_CONFIG="Release"
fi

if [ "$APP" = "sandbox" ]; then
    APP_NAME="Sandbox"
    EXECUTABLE="Sandbox"
elif [ "$APP" = "engine" ]; then
    APP_NAME="Engine"
    EXECUTABLE="Engine"
fi

# Construct the path to the executable
EXECUTABLE_PATH="Build/${BUILD_CONFIG}-linux-x86_64/${APP_NAME}/${EXECUTABLE}"

echo "Application: $APP_NAME"
echo "Configuration: $BUILD_CONFIG"
echo "Executable: $EXECUTABLE_PATH"
echo ""

# Check if the executable exists
if [ ! -f "$EXECUTABLE_PATH" ]; then
    echo "ERROR: Executable not found at: $EXECUTABLE_PATH"
    echo ""
    echo "Make sure you have built the project first:"
    echo "  ./Scripts/Linux/build.sh"
    echo ""
    echo "Or build the specific configuration:"
    echo "  make config=$CONFIG -j$(nproc)"
    exit 1
fi

# Check if executable is actually executable
if [ ! -x "$EXECUTABLE_PATH" ]; then
    echo "Making executable runnable..."
    chmod +x "$EXECUTABLE_PATH"
fi

# Set library path for SDL3 and other dependencies
export LD_LIBRARY_PATH="$PROJECT_ROOT/Engine/Vendor/SDL3/install/lib:$LD_LIBRARY_PATH"

echo "Starting $APP_NAME ($BUILD_CONFIG)..."
echo "Working directory: $PROJECT_ROOT"
echo "Library path: $LD_LIBRARY_PATH"
echo ""

# Run the application
./"$EXECUTABLE_PATH"
EXIT_CODE=$?

echo ""
echo "$APP_NAME exited with code: $EXIT_CODE"

exit $EXIT_CODE