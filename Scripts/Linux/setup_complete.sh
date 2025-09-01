#!/bin/bash
set -e

echo "Setting up Vortex Engine on Linux..."

# Get the script directory and navigate to project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
cd "$PROJECT_ROOT"

echo "Project root: $PROJECT_ROOT"

# Function to detect Linux distribution
detect_distro() {
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        echo $ID
    elif [ -f /etc/redhat-release ]; then
        echo "rhel"
    elif [ -f /etc/debian_version ]; then
        echo "debian"
    else
        echo "unknown"
    fi
}

DISTRO=$(detect_distro)
echo "Detected Linux distribution: $DISTRO"

# Function to install system dependencies
install_dependencies() {
    echo "Installing system dependencies..."
    
    case $DISTRO in
        ubuntu|debian|linuxmint)
            echo "Installing dependencies for Ubuntu/Debian/Linux Mint..."
            sudo apt-get update
            sudo apt-get install -y \
                build-essential \
                cmake \
                git \
                python3 \
                python3-pip \
                ninja-build \
                pkg-config \
                libx11-dev \
                libxrandr-dev \
                libxinerama-dev \
                libxcursor-dev \
                libxi-dev \
                libxext-dev \
                libwayland-dev \
                libxkbcommon-dev \
                libegl1-mesa-dev \
                libgl1-mesa-dev \
                libglu1-mesa-dev \
                libasound2-dev \
                libpulse-dev \
                libdbus-1-dev \
                libudev-dev \
                libusb-1.0-0-dev \
                wget \
                tar
            ;;
        fedora|rhel|centos)
            echo "Installing dependencies for RHEL/Fedora/CentOS..."
            if command -v dnf &> /dev/null; then
                PKG_MGR="dnf"
            else
                PKG_MGR="yum"
            fi
            
            sudo $PKG_MGR groupinstall -y "Development Tools"
            sudo $PKG_MGR install -y \
                cmake \
                git \
                python3 \
                python3-pip \
                ninja-build \
                pkgconfig \
                libX11-devel \
                libXrandr-devel \
                libXinerama-devel \
                libXcursor-devel \
                libXi-devel \
                libXext-devel \
                wayland-devel \
                libxkbcommon-devel \
                mesa-libEGL-devel \
                mesa-libGL-devel \
                mesa-libGLU-devel \
                alsa-lib-devel \
                pulseaudio-libs-devel \
                dbus-devel \
                systemd-devel \
                libusb1-devel \
                wget
            ;;
        arch|manjaro)
            echo "Installing dependencies for Arch Linux..."
            sudo pacman -Syu --noconfirm
            sudo pacman -S --noconfirm \
                base-devel \
                cmake \
                git \
                python \
                python-pip \
                ninja \
                pkg-config \
                libx11 \
                libxrandr \
                libxinerama \
                libxcursor \
                libxi \
                libxext \
                wayland \
                libxkbcommon \
                mesa \
                alsa-lib \
                libpulse \
                dbus \
                systemd \
                libusb \
                wget
            ;;
        opensuse*)
            echo "Installing dependencies for openSUSE..."
            sudo zypper refresh
            sudo zypper install -y \
                gcc-c++ \
                cmake \
                git \
                python3 \
                python3-pip \
                ninja \
                pkg-config \
                libX11-devel \
                libXrandr-devel \
                libXinerama-devel \
                libXcursor-devel \
                libXi-devel \
                libXext-devel \
                wayland-devel \
                libxkbcommon-devel \
                Mesa-libEGL-devel \
                Mesa-libGL-devel \
                Mesa-libGLU-devel \
                alsa-devel \
                libpulse-devel \
                dbus-1-devel \
                systemd-devel \
                libusb-1_0-devel \
                wget
            ;;
        *)
            echo "Unknown distribution. Please install the following manually:"
            echo "- build-essential (gcc, g++, make)"
            echo "- cmake"
            echo "- git"
            echo "- python3 and pip"
            echo "- ninja-build"
            echo "- X11 and OpenGL development headers"
            echo "- Audio libraries (ALSA/PulseAudio)"
            echo "- wget"
            echo ""
            read -p "Press Enter to continue after installing dependencies, or Ctrl+C to exit..."
            ;;
    esac
}

# Install system dependencies
install_dependencies

# Check if Premake5 exists, if not download it
echo "Setting up Premake5..."
if [ ! -f "Vendor/Premake/premake5" ]; then
    echo "Premake5 not found. Downloading..."
    mkdir -p "Vendor/Premake"
    
    # Determine architecture
    ARCH=$(uname -m)
    if [ "$ARCH" = "x86_64" ]; then
        PREMAKE_URL="https://github.com/premake/premake-core/releases/download/v5.0.0-beta2/premake-5.0.0-beta2-linux.tar.gz"
    else
        echo "Unsupported architecture: $ARCH. Please download Premake5 manually."
        exit 1
    fi
    
    echo "Downloading Premake5 for Linux ($ARCH)..."
    wget -O "Vendor/Premake/premake.tar.gz" "$PREMAKE_URL"
    
    if [ $? -ne 0 ]; then
        echo "Failed to download Premake5!"
        exit 1
    fi
    
    echo "Extracting Premake5..."
    cd "Vendor/Premake"
    tar -xzf "premake.tar.gz"
    rm "premake.tar.gz"
    cd "$PROJECT_ROOT"
    
    echo "Premake5 installed successfully!"
else
    echo "Premake5 found!"
fi

# Make premake5 executable
chmod +x "Vendor/Premake/premake5"

# Setup spdlog
echo "Setting up spdlog..."
if [ ! -d "Engine/Vendor/spdlog" ]; then
    echo "Cloning spdlog..."
    git clone --depth 1 https://github.com/gabime/spdlog.git "Engine/Vendor/spdlog"
    
    if [ $? -ne 0 ]; then
        echo "Failed to clone spdlog! Make sure git is installed."
        exit 1
    fi
    
    echo "spdlog cloned successfully!"
else
    echo "Updating spdlog..."
    cd "Engine/Vendor/spdlog"
    # Reset any local changes and pull latest
    git reset --hard HEAD
    git clean -fd
    git pull origin v1.x
    cd "$PROJECT_ROOT"
    echo "spdlog updated successfully!"
fi

# Setup SDL3
echo "Setting up SDL3..."
if [ ! -d "Engine/Vendor/SDL3" ]; then
    echo "Cloning SDL3..."
    git clone --depth 1 https://github.com/libsdl-org/SDL.git "Engine/Vendor/SDL3"
    
    if [ $? -ne 0 ]; then
        echo "Failed to clone SDL3! Make sure git is installed."
        exit 1
    fi
    
    echo "SDL3 cloned successfully!"
else
    echo "Updating SDL3..."
    cd "Engine/Vendor/SDL3"
    # Reset any local changes and pull latest
    git reset --hard HEAD
    git clean -fd
    git pull origin main
    cd "$PROJECT_ROOT"
    echo "SDL3 updated successfully!"
fi

# Build SDL3 statically with CMake
echo "Building SDL3 statically..."
cd "Engine/Vendor/SDL3"

# Clean any previous build
if [ -d "build" ]; then
    echo "Cleaning previous SDL3 build..."
    rm -rf build
fi

mkdir -p "build"
cd "build"

# Configure SDL3 build for Linux
echo "Configuring SDL3 build for Linux..."
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DSDL_SHARED=OFF \
    -DSDL_STATIC=ON \
    -DSDL_TEST_LIBRARY=OFF \
    -DSDL_TESTS=OFF \
    -DSDL_EXAMPLES=OFF \
    -DSDL_WAYLAND=ON \
    -DSDL_X11=ON \
    -DSDL_PULSEAUDIO=ON \
    -DSDL_ALSA=ON \
    -DSDL_DBUS=ON \
    -DSDL_IME=ON \
    -DSDL_OPENGL=ON \
    -DSDL_OPENGLES=OFF \
    -DSDL_VULKAN=OFF \
    -DSDL_METAL=OFF \
    -DSDL_DIRECTFB=OFF \
    -DSDL_OPENGLES2=OFF \
    -DSDL_OPENGLES3=OFF \
    -DCMAKE_INSTALL_PREFIX="../install"

if [ $? -ne 0 ]; then
    echo "Failed to configure SDL3 build!"
    echo "This might be due to missing dependencies. Please ensure all required packages are installed."
    exit 1
fi

# Build SDL3
echo "Building SDL3..."
make -j$(nproc)

if [ $? -ne 0 ]; then
    echo "Failed to build SDL3!"
    echo "This might be due to missing dependencies or build issues."
    exit 1
fi

# Install SDL3
echo "Installing SDL3..."
make install

if [ $? -ne 0 ]; then
    echo "Failed to install SDL3!"
    exit 1
fi

cd "$PROJECT_ROOT"
echo "SDL3 built and installed successfully!"

# Verify SDL3 installation
echo "Verifying SDL3 installation..."
if [ -f "Engine/Vendor/SDL3/install/lib/libSDL3.a" ]; then
    echo "SDL3 static library found: Engine/Vendor/SDL3/install/lib/libSDL3.a"
else
    echo "ERROR: SDL3 static library not found!"
    exit 1
fi

# Setup GLM (OpenGL Mathematics)
echo "Setting up GLM..."
if [ ! -d "Engine/Vendor/glm" ]; then
    echo "Cloning GLM..."
    git clone --depth 1 https://github.com/g-truc/glm.git "Engine/Vendor/glm"
    
    if [ $? -ne 0 ]; then
        echo "Failed to clone GLM! Make sure git is installed."
        exit 1
    fi
    
    echo "GLM cloned successfully!"
else
    echo "Updating GLM..."
    cd "Engine/Vendor/glm"
    # Reset any local changes and pull latest
    git reset --hard HEAD
    git clean -fd
    git pull origin master
    cd "$PROJECT_ROOT"
    echo "GLM updated successfully!"
fi

# Setup nlohmann/json (JSON Library)
echo "Setting up nlohmann/json..."
if [ ! -d "Engine/Vendor/nlohmann_json" ]; then
    echo "Cloning nlohmann/json..."
    git clone --depth 1 --branch develop https://github.com/nlohmann/json.git "Engine/Vendor/nlohmann_json"
    
    if [ $? -ne 0 ]; then
        echo "Failed to clone nlohmann/json! Make sure git is installed."
        exit 1
    fi
    
    echo "nlohmann/json cloned successfully!"
else
    echo "Updating nlohmann/json..."
    cd "Engine/Vendor/nlohmann_json"
    # Reset any local changes and pull latest
    git reset --hard HEAD
    git clean -fd
    git pull origin develop
    cd "$PROJECT_ROOT"
    echo "nlohmann/json updated successfully!"
fi

# Setup GLAD
echo "Setting up GLAD..."
if [ ! -d "Engine/Vendor/GLAD" ]; then
    echo "Cloning GLAD..."
    git clone --depth 1 https://github.com/Dav1dde/glad.git "Engine/Vendor/GLAD"
    
    if [ $? -ne 0 ]; then
        echo "Failed to clone GLAD! Make sure git is installed."
        exit 1
    fi
    
    echo "GLAD cloned successfully!"
else
    echo "Updating GLAD..."
    cd "Engine/Vendor/GLAD"
    # Completely reset and force update to handle divergent branches
    git fetch origin
    git reset --hard origin/glad2
    git clean -fd
    cd "$PROJECT_ROOT"
    echo "GLAD updated successfully!"
fi

# Generate GLAD files for OpenGL 4.6 Core Profile
echo "Generating GLAD files for OpenGL 4.6 Core Profile..."
cd "Engine/Vendor/GLAD"

# Create generated directory if it doesn't exist
mkdir -p "generated"

# Check if Python is available
echo "Checking for Python..."
PYTHON_CMD=""

if command -v python3 &> /dev/null; then
    PYTHON_CMD="python3"
    echo "Python found: python3"
elif command -v python &> /dev/null; then
    PYTHON_CMD="python"
    echo "Python found: python"
else
    echo "Python not found! Please install Python 3."
    exit 1
fi

# Install required Python packages
echo "Installing required Python packages..."
echo "Attempting to install jinja2 and glad2..."

# Try different installation methods
if $PYTHON_CMD -m pip install --user jinja2 glad2; then
    echo "Python packages installed successfully!"
elif $PYTHON_CMD -m pip install --user --break-system-packages jinja2 glad2; then
    echo "Python packages installed successfully with system override!"
else
    echo "Failed to install Python packages with pip. Trying alternative methods..."
    
    # Try installing with sudo (not recommended but sometimes necessary)
    if sudo $PYTHON_CMD -m pip install jinja2 glad2; then
        echo "Python packages installed successfully with sudo!"
    else
        echo "ERROR: Failed to install required Python packages!"
        echo "Please install them manually:"
        echo "  $PYTHON_CMD -m pip install --user jinja2 glad2"
        echo "Or with system override:"
        echo "  $PYTHON_CMD -m pip install --user --break-system-packages jinja2 glad2"
        exit 1
    fi
fi

# Generate GLAD
echo "Generating GLAD for OpenGL 4.6 Core..."
$PYTHON_CMD -m glad --api="gl:core=4.6" --out-path=generated c

if [ $? -ne 0 ]; then
    echo "Failed to generate GLAD files!"
    echo "This might be due to missing Python packages or glad2 installation issues."
    exit 1
fi

# Verify GLAD generation
echo "Verifying GLAD generation..."
if [ -f "generated/include/glad/gl.h" ] && [ -f "generated/src/gl.c" ]; then
    echo "GLAD files generated successfully!"
    echo "  - Header: generated/include/glad/gl.h"
    echo "  - Source: generated/src/gl.c"
else
    echo "ERROR: GLAD files not found after generation!"
    exit 1
fi

cd "$PROJECT_ROOT"
echo "GLAD generated successfully!"

# Generate build files with Premake5
echo "Generating build files..."
./Vendor/Premake/premake5 gmake2

if [ $? -ne 0 ]; then
    echo "Failed to generate build files!"
    exit 1
fi

echo "Build files generated successfully!"

# Final verification
echo ""
echo "Performing final verification..."
echo ""

# Check if all required files exist
echo "Checking required files:"

# Check SDL3
if [ -f "Engine/Vendor/SDL3/install/lib/libSDL3.a" ]; then
    echo "✅ SDL3 static library found"
else
    echo "❌ SDL3 static library missing"
    exit 1
fi

# Check GLAD
if [ -f "Engine/Vendor/GLAD/generated/include/glad/gl.h" ] && [ -f "Engine/Vendor/GLAD/generated/src/gl.c" ]; then
    echo "✅ GLAD files found"
else
    echo "❌ GLAD files missing"
    exit 1
fi

# Check other dependencies
if [ -d "Engine/Vendor/spdlog" ]; then
    echo "✅ spdlog found"
else
    echo "❌ spdlog missing"
    exit 1
fi

if [ -d "Engine/Vendor/glm" ]; then
    echo "✅ GLM found"
else
    echo "❌ GLM missing"
    exit 1
fi

if [ -d "Engine/Vendor/nlohmann_json" ]; then
    echo "✅ nlohmann/json found"
else
    echo "❌ nlohmann/json missing"
    exit 1
fi

# Check build files
if [ -f "Engine/Makefile" ] && [ -f "Sandbox/Makefile" ]; then
    echo "✅ Build files generated"
else
    echo "❌ Build files missing"
    exit 1
fi

echo ""
echo "🎉 Setup completed successfully!"
echo ""
echo "All dependencies are installed and configured for Linux."
echo ""
echo "To build the project, run:"
echo "  ./Scripts/Linux/build.sh"
echo ""
echo "Or use make directly:"
echo "  make config=debug -j$(nproc)"
echo "  make config=release -j$(nproc)"
echo ""
echo "To clean the build:"
echo "  ./Scripts/Linux/clean.sh"
echo ""
echo "To run the application:"
echo "  ./Scripts/Linux/run.sh"
echo ""
echo "Happy coding! 🚀"