@echo off
setlocal EnableDelayedExpansion
echo Setting up Vortex Engine...

:: Navigate to project root (go up two directories from Scripts\Windows)
pushd "%~dp0..\.."

:: Check if Premake5 exists, if not download it
if not exist "Vendor\Premake\premake5.exe" (
    echo Premake5 not found. Downloading...
    if not exist "Vendor\Premake" mkdir "Vendor\Premake"
    
    echo Downloading Premake5 for Windows...
    powershell -Command "Invoke-WebRequest -Uri 'https://github.com/premake/premake-core/releases/download/v5.0.0-beta2/premake-5.0.0-beta2-windows.zip' -OutFile 'Vendor\Premake\premake.zip'"
    
    if %ERRORLEVEL% NEQ 0 (
        echo Failed to download Premake5!
        popd
        pause
        exit /b 1
    )
    
    echo Extracting Premake5...
    powershell -Command "Expand-Archive -Path 'Vendor\Premake\premake.zip' -DestinationPath 'Vendor\Premake' -Force"
    del "Vendor\Premake\premake.zip"
    
    echo Premake5 installed successfully!
)

:: Check if CMake is installed
echo Checking for CMake...
cmake --version >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo CMake not found. Installing CMake...
    if not exist "Vendor\CMake" mkdir "Vendor\CMake"
    
    echo Downloading CMake for Windows...
    powershell -Command "Invoke-WebRequest -Uri 'https://github.com/Kitware/CMake/releases/download/v3.27.7/cmake-3.27.7-windows-x86_64.zip' -OutFile 'Vendor\CMake\cmake.zip'"
    
    if %ERRORLEVEL% NEQ 0 (
        echo Failed to download CMake!
        popd
        pause
        exit /b 1
    )
    
    echo Extracting CMake...
    powershell -Command "Expand-Archive -Path 'Vendor\CMake\cmake.zip' -DestinationPath 'Vendor\CMake' -Force"
    del "Vendor\CMake\cmake.zip"
    
    :: Add CMake to PATH for this session
    set "PATH=%CD%\Vendor\CMake\cmake-3.27.7-windows-x86_64\bin;%PATH%"
    
    echo CMake installed successfully!
) else (
    echo CMake found!
)

:: Setup spdlog
echo Setting up spdlog...
if not exist "Engine\Vendor\spdlog" (
    echo Cloning spdlog...
    git clone --depth 1 https://github.com/gabime/spdlog.git "Engine\Vendor\spdlog"
    
    if %ERRORLEVEL% NEQ 0 (
        echo Failed to clone spdlog! Make sure git is installed.
        popd
        pause
        exit /b 1
    )
    
    echo spdlog cloned successfully!
) else (
    echo Updating spdlog...
    cd "Engine\Vendor\spdlog"
    git pull origin master
    cd "..\..\.."
    echo spdlog updated successfully!
)

:: Setup SDL3
echo Setting up SDL3...
if not exist "Engine\Vendor\SDL3" (
    echo Cloning SDL3...
    git clone --depth 1 https://github.com/libsdl-org/SDL.git "Engine\Vendor\SDL3"
    
    if %ERRORLEVEL% NEQ 0 (
        echo Failed to clone SDL3! Make sure git is installed.
        popd
        pause
        exit /b 1
    )
    
    echo SDL3 cloned successfully!
) else (
    echo Updating SDL3...
    cd "Engine\Vendor\SDL3"
    git pull origin main
    cd "..\..\.."
    echo SDL3 updated successfully!
)

:: Build SDL3 statically with CMake
echo Building SDL3 statically...
if not exist "Engine\Vendor\SDL3\build" mkdir "Engine\Vendor\SDL3\build"
cd "Engine\Vendor\SDL3\build"

:: Configure SDL3 build
echo Configuring SDL3 build...
cmake .. -G "Visual Studio 17 2022" -A x64 ^
    -DSDL_SHARED=OFF ^
    -DSDL_STATIC=ON ^
    -DSDL_TEST_LIBRARY=OFF ^
    -DSDL_TESTS=OFF ^
    -DSDL_EXAMPLES=OFF ^
    -DCMAKE_MSVC_RUNTIME_LIBRARY="MultiThreaded$<$<CONFIG:Debug>:Debug>" ^
    -DCMAKE_INSTALL_PREFIX="../install"

if %ERRORLEVEL% NEQ 0 (
    echo Failed to configure SDL3 build!
    cd "..\..\..\.."
    popd
    pause
    exit /b 1
)

:: Build SDL3 in both Debug and Release
echo Building SDL3 Debug configuration...
cmake --build . --config Debug

if %ERRORLEVEL% NEQ 0 (
    echo Failed to build SDL3 Debug!
    cd "..\..\..\.."
    popd
    pause
    exit /b 1
)

echo Building SDL3 Release configuration...
cmake --build . --config Release

if %ERRORLEVEL% NEQ 0 (
    echo Failed to build SDL3 Release!
    cd "..\..\..\.."
    popd
    pause
    exit /b 1
)

:: Install SDL3 to configuration-specific directories
echo Installing SDL3 (Debug -> install/Debug, Release -> install/Release)...
cmake --install . --config Debug --prefix "../install/Debug"
cmake --install . --config Release --prefix "../install/Release"

cd "..\..\..\.."
echo SDL3 built and installed successfully!

:: Setup GLM (OpenGL Mathematics)
echo Setting up GLM...
if not exist "Engine\Vendor\glm" (
    echo Cloning GLM...
    git clone --depth 1 https://github.com/g-truc/glm.git "Engine\Vendor\glm"
    
    if %ERRORLEVEL% NEQ 0 (
        echo Failed to clone GLM! Make sure git is installed.
        popd
        pause
        exit /b 1
    )
    
    echo GLM cloned successfully!
) else (
    echo Updating GLM...
    cd "Engine\Vendor\glm"
    git pull origin master
    cd "..\..\.."
    echo GLM updated successfully!
)

:: Setup nlohmann/json (JSON Library)
echo Setting up nlohmann/json...
if not exist "Engine\Vendor\nlohmann_json" (
    echo Cloning nlohmann/json...
    git clone --depth 1 --branch develop https://github.com/nlohmann/json.git "Engine\Vendor\nlohmann_json"
    
    if %ERRORLEVEL% NEQ 0 (
        echo Failed to clone nlohmann/json! Make sure git is installed.
        popd
        pause
        exit /b 1
    )
    
    echo nlohmann/json cloned successfully!
) else (
    echo Updating nlohmann/json...
    cd "Engine\Vendor\nlohmann_json"
    git pull origin develop
    cd "..\..\.."
    echo nlohmann/json updated successfully!
)

:: Setup Vulkan SDK
echo Setting up Vulkan SDK...
set VULKAN_SDK_VERSION=1.3.280.0

:: Check if Vulkan SDK is already installed
if defined VULKAN_SDK (
    if exist "%VULKAN_SDK%\Bin\vulkaninfo.exe" (
        echo Vulkan SDK found via system installation!
        echo Using: %VULKAN_SDK%
        goto :vulkan_found
    )
)

:: Check for local Vulkan SDK installation
if not exist "Vendor\VulkanSDK" (
    echo Vulkan SDK not found. Setting up local installation...
    mkdir "Vendor\VulkanSDK"
    
    echo Downloading Vulkan SDK %VULKAN_SDK_VERSION% for Windows...
    powershell -Command "Invoke-WebRequest -Uri 'https://sdk.lunarg.com/sdk/download/1.3.280.0/windows/VulkanSDK-1.3.280.0-Installer.exe' -OutFile 'Vendor\VulkanSDK\VulkanSDK-Installer.exe'"
    
    if %ERRORLEVEL% NEQ 0 (
        echo Failed to download Vulkan SDK! Trying to use minimal setup...
        echo Setting up minimal Vulkan headers and libraries...
        
        :: Download Vulkan headers
        if not exist "Vendor\VulkanSDK\Include\vulkan" mkdir "Vendor\VulkanSDK\Include\vulkan"
        echo Downloading Vulkan headers...
        powershell -Command "Invoke-WebRequest -Uri 'https://raw.githubusercontent.com/KhronosGroup/Vulkan-Headers/main/include/vulkan/vulkan.h' -OutFile 'Vendor\VulkanSDK\Include\vulkan\vulkan.h'"
        powershell -Command "Invoke-WebRequest -Uri 'https://raw.githubusercontent.com/KhronosGroup/Vulkan-Headers/main/include/vulkan/vulkan_core.h' -OutFile 'Vendor\VulkanSDK\Include\vulkan\vulkan_core.h'"
        
        goto :skip_vulkan_installer
    )
    
    echo Please run the Vulkan SDK installer manually: Vendor\VulkanSDK\VulkanSDK-Installer.exe
    echo After installation, re-run this setup script.
    echo.
    echo Note: You can also install Vulkan SDK system-wide and it will be detected automatically.
    pause
    popd
    exit /b 1
) else (
    echo Local Vulkan SDK directory found.
)

:skip_vulkan_installer

:: Set local Vulkan SDK path for this session
if exist "Vendor\VulkanSDK\Include" (
    set "VULKAN_SDK=%CD%\Vendor\VulkanSDK"
    echo Local Vulkan SDK path set: !VULKAN_SDK!
)

:vulkan_found
echo Vulkan SDK setup complete.

:: Check for Python early since it's needed for shaderc
echo Checking for Python...
echo Current PATH: %PATH%
echo.

:: Try multiple ways to find Python
set PYTHON_FOUND=0

:: Method 1: Try python command
echo Trying 'python' command...
python --version >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    set PYTHON_FOUND=1
    set PYTHON_CMD=python
    echo Python found via 'python' command!
    python --version
)

:: Method 2: Try python3 command
if %PYTHON_FOUND% EQU 0 (
    echo Trying 'python3' command...
    python3 --version >nul 2>&1
    if %ERRORLEVEL% EQU 0 (
        set PYTHON_FOUND=1
        set PYTHON_CMD=python3
        echo Python found via 'python3' command!
        python3 --version
    )
)

:: Method 3: Try py launcher
if %PYTHON_FOUND% EQU 0 (
    echo Trying 'py' launcher...
    py --version >nul 2>&1
    if %ERRORLEVEL% EQU 0 (
        set PYTHON_FOUND=1
        set PYTHON_CMD=py
        echo Python found via 'py' launcher!
        py --version
    )
)

:: Method 4: Search common installation paths
if %PYTHON_FOUND% EQU 0 (
    echo Searching common Python installation paths...
    
    :: Check %LOCALAPPDATA%\Programs\Python
    if exist "%LOCALAPPDATA%\Programs\Python" (
        echo Found Python directory: %LOCALAPPDATA%\Programs\Python
        for /d %%i in ("%LOCALAPPDATA%\Programs\Python\Python*") do (
            if exist "%%i\python.exe" (
                set PYTHON_FOUND=1
                set PYTHON_CMD="%%i\python.exe"
                echo Python found at: %%i\python.exe
                "%%i\python.exe" --version
                goto :python_found_early
            )
        )
    )
    
    :: Check %APPDATA%\Local\Programs\Python
    if exist "%APPDATA%\Local\Programs\Python" (
        echo Found Python directory: %APPDATA%\Local\Programs\Python
        for /d %%i in ("%APPDATA%\Local\Programs\Python\Python*") do (
            if exist "%%i\python.exe" (
                set PYTHON_FOUND=1
                set PYTHON_CMD="%%i\python.exe"
                echo Python found at: %%i\python.exe
                "%%i\python.exe" --version
                goto :python_found_early
            )
        )
    )
    
    :: Check C:\Python*
    if exist "C:\Python*" (
        for /d %%i in ("C:\Python*") do (
            if exist "%%i\python.exe" (
                set PYTHON_FOUND=1
                set PYTHON_CMD="%%i\python.exe"
                echo Python found at: %%i\python.exe
                "%%i\python.exe" --version
                goto :python_found_early
            )
        )
    )
)

:python_found_early
if %PYTHON_FOUND% EQU 0 (
    echo.
    echo Python not found! 
    echo Please install Python from https://www.python.org/downloads/
    echo Make sure to check "Add Python to PATH" during installation.
    echo.
    echo You may need to restart your command prompt after installing Python.
    echo After installing Python, re-run this setup script.
    popd
    pause
    exit /b 1
) else (
    echo.
    echo Python successfully detected!
    echo Using: %PYTHON_CMD%
    echo.
)

:: Setup SPIRV-Headers (dependency for shaderc)
echo Setting up SPIRV-Headers...
if not exist "Engine\Vendor\SPIRV-Headers" (
    echo Cloning SPIRV-Headers...
    git clone --depth 1 https://github.com/KhronosGroup/SPIRV-Headers.git "Engine\Vendor\SPIRV-Headers"
    
    if %ERRORLEVEL% NEQ 0 (
        echo Failed to clone SPIRV-Headers! Make sure git is installed.
        popd
        pause
        exit /b 1
    )
    
    echo SPIRV-Headers cloned successfully!
) else (
    echo Updating SPIRV-Headers...
    cd "Engine\Vendor\SPIRV-Headers"
    git pull origin main
    cd "..\..\.."
    echo SPIRV-Headers updated successfully!
)

:: Setup SPIRV-Tools (dependency for shaderc)
echo Setting up SPIRV-Tools...
if not exist "Engine\Vendor\SPIRV-Tools" (
    echo Cloning SPIRV-Tools...
    git clone --recurse-submodules https://github.com/KhronosGroup/SPIRV-Tools.git "Engine\Vendor\SPIRV-Tools"
    
    if %ERRORLEVEL% NEQ 0 (
        echo Failed to clone SPIRV-Tools! Make sure git is installed.
        popd
        pause
        exit /b 1
    )
    
    echo SPIRV-Tools cloned successfully!
) else (
    echo Updating SPIRV-Tools...
    cd "Engine\Vendor\SPIRV-Tools"
    git pull origin main
    git submodule update --init --recursive
    cd "..\..\.."
    echo SPIRV-Tools updated successfully!
)

:: Build SPIRV-Tools
echo Building SPIRV-Tools...
cd "Engine\Vendor\SPIRV-Tools"

:: Ensure SPIRV-Headers is available in external without requiring symlink privileges
if not exist "external" mkdir "external"
if exist "external\spirv-headers" (
    echo Refreshing local copy of SPIRV-Headers for SPIRV-Tools...
    rmdir /S /Q "external\spirv-headers"
)

echo Copying SPIRV-Headers into SPIRV-Tools/external...
robocopy "..\SPIRV-Headers" "external\spirv-headers" /E /NFL /NDL /NJH /NJS /NP >nul
if %ERRORLEVEL% GEQ 8 (
    echo Failed to copy SPIRV-Headers!
    cd "..\..\..\.."
    popd
    pause
    exit /b 1
)

if not exist "build" mkdir "build"
cd "build"

echo Configuring SPIRV-Tools build...
cmake .. -G "Visual Studio 17 2022" -A x64 ^
    -DSPIRV_SKIP_TESTS=ON ^
    -DSPIRV_SKIP_EXECUTABLES=OFF ^
    -DBUILD_SHARED_LIBS=OFF ^
    -DCMAKE_MSVC_RUNTIME_LIBRARY="MultiThreaded$<$<CONFIG:Debug>:Debug>" ^
    -DCMAKE_CXX_FLAGS_DEBUG="/MTd" ^
    -DCMAKE_CXX_FLAGS_RELEASE="/MT" ^
    -DCMAKE_C_FLAGS_DEBUG="/MTd" ^
    -DCMAKE_C_FLAGS_RELEASE="/MT" ^
    -DCMAKE_INSTALL_PREFIX="../install"

if %ERRORLEVEL% NEQ 0 (
    echo Failed to configure SPIRV-Tools build!
    cd "..\..\..\.."
    popd
    pause
    exit /b 1
)

echo Building SPIRV-Tools Debug configuration...
cmake --build . --config Debug

if %ERRORLEVEL% NEQ 0 (
    echo Failed to build SPIRV-Tools Debug!
    cd "..\..\..\.."
    popd
    pause
    exit /b 1
)

echo Building SPIRV-Tools Release configuration...
cmake --build . --config Release

if %ERRORLEVEL% NEQ 0 (
    echo Failed to build SPIRV-Tools Release!
    cd "..\..\..\.."
    popd
    pause
    exit /b 1
)

echo Installing SPIRV-Tools...
cmake --install . --config Debug --prefix "../install/Debug"
cmake --install . --config Release --prefix "../install/Release"

cd "..\..\..\.."
echo SPIRV-Tools built and installed successfully!

:: Setup shaderc (Google's shader compiler)
echo Setting up shaderc...
if not exist "Engine\Vendor\shaderc" (
    echo Cloning shaderc...
    git clone https://github.com/google/shaderc.git "Engine\Vendor\shaderc"
    
    if %ERRORLEVEL% NEQ 0 (
        echo Failed to clone shaderc! Make sure git is installed.
        popd
        pause
        exit /b 1
    )
    
    echo shaderc cloned successfully!
) else (
    echo Updating shaderc...
    cd "Engine\Vendor\shaderc"
    git pull origin main
    cd "..\..\.."
    echo shaderc updated successfully!
)

:: Setup shaderc dependencies using git-sync-deps
echo Setting up shaderc dependencies...
cd "Engine\Vendor\shaderc"

:: Check if Python is available (we know it exists from GLAD setup)
echo Running git-sync-deps to fetch shaderc dependencies...
%PYTHON_CMD% utils/git-sync-deps

if %ERRORLEVEL% NEQ 0 (
    echo Failed to sync shaderc dependencies!
    cd "..\..\.."
    popd
    pause
    exit /b 1
)

cd "..\..\.."
echo shaderc dependencies synced successfully!

:: Build shaderc
echo Building shaderc...
if not exist "Engine\Vendor\shaderc\build" mkdir "Engine\Vendor\shaderc\build"
cd "Engine\Vendor\shaderc\build"

echo Configuring shaderc build...
cmake .. -G "Visual Studio 17 2022" -A x64 ^
    -DSHADERC_SKIP_TESTS=ON ^
    -DSHADERC_SKIP_EXAMPLES=ON ^
    -DSHADERC_SKIP_COPYRIGHT_CHECK=ON ^
    -DBUILD_SHARED_LIBS=OFF ^
    -DSHADERC_ENABLE_SHARED_CRT=OFF ^
    -DCMAKE_MSVC_RUNTIME_LIBRARY="MultiThreaded$<$<CONFIG:Debug>:Debug>" ^
    -DCMAKE_CXX_FLAGS_DEBUG="/MTd" ^
    -DCMAKE_CXX_FLAGS_RELEASE="/MT" ^
    -DCMAKE_C_FLAGS_DEBUG="/MTd" ^
    -DCMAKE_C_FLAGS_RELEASE="/MT" ^
    -DCMAKE_INSTALL_PREFIX="../install"

if %ERRORLEVEL% NEQ 0 (
    echo Failed to configure shaderc build!
    cd "..\..\..\.."
    popd
    pause
    exit /b 1
)

echo Building shaderc Debug configuration...
cmake --build . --config Debug

if %ERRORLEVEL% NEQ 0 (
    echo Failed to build shaderc Debug!
    cd "..\..\..\.."
    popd
    pause
    exit /b 1
)

echo Building shaderc Release configuration...
cmake --build . --config Release

if %ERRORLEVEL% NEQ 0 (
    echo Failed to build shaderc Release!
    cd "..\..\..\.."
    popd
    pause
    exit /b 1
)

echo Installing shaderc...
cmake --install . --config Debug --prefix "../install/Debug"
cmake --install . --config Release --prefix "../install/Release"

cd "..\..\..\.."
echo shaderc built and installed successfully!

:: Setup GLAD
if not exist "Engine\Vendor\GLAD" (
    echo Cloning GLAD...
    git clone --depth 1 https://github.com/Dav1dde/glad.git "Engine\Vendor\GLAD"
    
    if %ERRORLEVEL% NEQ 0 (
        echo Failed to clone GLAD! Make sure git is installed.
        popd
        pause
        exit /b 1
    )
    
    echo GLAD cloned successfully!
) else (
    echo Updating GLAD...
    cd "Engine\Vendor\GLAD"
    git pull origin master
    cd "..\..\.."
    echo GLAD updated successfully!
)

:: Generate GLAD files for OpenGL 4.6 Core Profile
echo Generating GLAD files for OpenGL 4.6 Core Profile...
cd "Engine\Vendor\GLAD"

:: Create generated directory if it doesn't exist
if not exist "generated" mkdir "generated"

:: Generate OpenGL 4.6 Core Profile with extensions
echo About to generate GLAD for OpenGL 4.6 Core...
echo Current directory: %CD%
echo Using Python command: %PYTHON_CMD%
echo Generating GLAD for OpenGL 4.6 Core...

echo DEBUG: About to run GLAD generation command...
%PYTHON_CMD% -m glad --api="gl:core=4.6" --out-path=generated c
set GLAD_ERROR=%ERRORLEVEL%
echo DEBUG: GLAD generation completed with error level: %GLAD_ERROR%

if %GLAD_ERROR% NEQ 0 (
    echo Failed to generate GLAD files!
    echo Installing required Python packages...
    
    :: Try different pip commands
    echo DEBUG: Looking for pip...
    set PIP_FOUND=0
    
    :: Try pip
    echo DEBUG: Trying pip command...
    pip --version >nul 2>&1
    if !ERRORLEVEL! EQU 0 (
        echo DEBUG: Found pip command
        set PIP_FOUND=1
        set "PIP_CMD=pip"
    ) else (
        :: Try python -m pip
        echo DEBUG: Trying python -m pip...
        %PYTHON_CMD% -m pip --version >nul 2>&1
        if !ERRORLEVEL! EQU 0 (
            echo DEBUG: Found python -m pip
            set PIP_FOUND=1
            set "PIP_CMD=%PYTHON_CMD% -m pip"
        ) else (
            echo DEBUG: python -m pip failed
        )
    )
    
    echo DEBUG: PIP_FOUND=%PIP_FOUND%
    if !PIP_FOUND! EQU 0 (
        echo pip not found! Please install pip or use a Python installation that includes pip.
        cd "..\..\.."
        popd
        pause
        exit /b 1
    )
    
    echo Using pip command: !PIP_CMD!
    echo Installing jinja2 and glad2...
    echo DEBUG: Running pip install command...
    !PIP_CMD! install jinja2 glad2 --user
    set PIP_ERROR=!ERRORLEVEL!
    echo DEBUG: pip install completed with error level: !PIP_ERROR!
    
    if !PIP_ERROR! NEQ 0 (
        echo Failed to install required packages!
        cd "..\..\.."
        popd
        pause
        exit /b 1
    )
    
    echo Required packages installed, retrying generation...
    echo DEBUG: Retrying GLAD generation...
    %PYTHON_CMD% -m glad --api="gl:core=4.6" --out-path=generated c
    set GLAD_RETRY_ERROR=!ERRORLEVEL!
    echo DEBUG: GLAD retry completed with error level: !GLAD_RETRY_ERROR!
    
    if !GLAD_RETRY_ERROR! NEQ 0 (
        echo Failed to generate GLAD files after installing packages!
        cd "..\..\.."
        popd
        pause
        exit /b 1
    )
)

cd "..\..\.."
echo GLAD generated successfully!

echo Generating Visual Studio project files...
call "Vendor\Premake\premake5.exe" vs2022

if %ERRORLEVEL% NEQ 0 (
    echo Failed to generate project files!
    popd
    pause
    exit /b 1
)

echo Project files generated successfully!
echo You can now open Vortex.sln in Visual Studio 2022

:: Return to original directory
popd
pause
