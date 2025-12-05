@echo off
setlocal EnableDelayedExpansion
echo ============================================================
echo                   Vortex Engine Setup
echo ============================================================
echo.

:: Install Prerequisites 
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0BackendScripts\InstallPrerequisites.ps1"

:: Navigate to project root (go up two directories from Scripts\Windows)
pushd "%~dp0..\.."

:: Check for non-interactive mode
if defined NONINTERACTIVE (
    echo Running in non-interactive mode - skipping update prompts...
    echo Only missing dependencies will be installed.
    echo.
) else (
    echo For each existing library, you will be prompted to update.
    echo Press Enter or 'n' to skip updates, 'y' to update.
    echo To install only missing dependencies, set NONINTERACTIVE=1 before running.
    echo.
)

:: ============================================================
:: Setup spdlog
:: ============================================================
echo Setting up spdlog...
if not exist "Engine\Vendor\spdlog" (
    echo Cloning spdlog...
    git clone --depth 1 https://github.com/gabime/spdlog.git "Engine\Vendor\spdlog"
    
    if !ERRORLEVEL! NEQ 0 (
        echo Failed to clone spdlog! Make sure git is installed.
        popd
        pause
        exit /b 1
    )
    
    echo spdlog cloned successfully!
) else (
    if not defined NONINTERACTIVE (
        set /p UPDATE_SPDLOG="spdlog already exists. Update? (y/N): "
        if /i "!UPDATE_SPDLOG!"=="y" (
            echo Updating spdlog...
            cd "Engine\Vendor\spdlog"
            git pull origin master
            cd "..\..\.."
            echo spdlog updated successfully!
        ) else (
            echo Skipping spdlog update.
        )
    ) else (
        echo spdlog already exists. Skipping.
    )
)

:: ============================================================
:: Setup SDL3
:: ============================================================
echo.
echo Setting up SDL3...
set SDL3_NEEDS_BUILD=0
set SDL3_IS_NEW=0
if not exist "Engine\Vendor\SDL3" (
    echo Cloning SDL3...
    git clone --depth 1 https://github.com/libsdl-org/SDL.git "Engine\Vendor\SDL3"
    
    if !ERRORLEVEL! NEQ 0 (
        echo Failed to clone SDL3! Make sure git is installed.
        popd
        pause
        exit /b 1
    )
    
    echo SDL3 cloned successfully!
    set SDL3_NEEDS_BUILD=1
    set SDL3_IS_NEW=1
) else (
    if not defined NONINTERACTIVE (
        set /p UPDATE_SDL3="SDL3 already exists. Update? (y/N): "
        if /i "!UPDATE_SDL3!"=="y" (
            echo Updating SDL3...
            cd "Engine\Vendor\SDL3"
            git pull origin main
            cd "..\..\.."
            echo SDL3 updated successfully!
            set SDL3_NEEDS_BUILD=1
        ) else (
            echo Skipping SDL3 update.
        )
    ) else (
        echo SDL3 already exists. Skipping.
    )
)

:: Check if SDL3 is built - look for any lib file in install directories
set SDL3_BUILT=0
if exist "Engine\Vendor\SDL3\install\Debug\lib\SDL3-static.lib" set SDL3_BUILT=1
if exist "Engine\Vendor\SDL3\install\Release\lib\SDL3-static.lib" set SDL3_BUILT=1
if exist "Engine\Vendor\SDL3\install\lib\SDL3-static.lib" set SDL3_BUILT=1
if exist "Engine\Vendor\SDL3\install\lib\SDL3.lib" set SDL3_BUILT=1

:: If SDL3 was just cloned, we need to build it
if !SDL3_IS_NEW! EQU 1 (
    set SDL3_NEEDS_BUILD=1
    goto :sdl3_do_build
)

:: If SDL3 was updated, we need to rebuild
if !SDL3_NEEDS_BUILD! EQU 1 (
    goto :sdl3_do_build
)

:: If SDL3 is not built, ask user if they want to build
if !SDL3_BUILT! EQU 0 (
    if not defined NONINTERACTIVE (
        set /p BUILD_SDL3="SDL3 is not built. Build now? (y/N): "
        if /i "!BUILD_SDL3!"=="y" (
            set SDL3_NEEDS_BUILD=1
            goto :sdl3_do_build
        ) else (
            echo Skipping SDL3 build. Note: Build may fail without SDL3!
            goto :sdl3_done
        )
    ) else (
        echo SDL3 not built. Building...
        set SDL3_NEEDS_BUILD=1
        goto :sdl3_do_build
    )
) else (
    :: SDL3 is already built, ask if user wants to rebuild
    if not defined NONINTERACTIVE (
        set /p REBUILD_SDL3="SDL3 is already built. Rebuild? (y/N): "
        if /i "!REBUILD_SDL3!"=="y" (
            goto :sdl3_rebuild
        ) else (
            echo Skipping SDL3 rebuild.
            goto :sdl3_done
        )
    ) else (
        echo SDL3 already built. Skipping.
        goto :sdl3_done
    )
)

:sdl3_do_build
echo Building SDL3 statically...
if not exist "Engine\Vendor\SDL3\build" mkdir "Engine\Vendor\SDL3\build"
cd "Engine\Vendor\SDL3\build"

echo Configuring SDL3 build...
cmake .. -G "Visual Studio 17 2022" -A x64 ^
    -DSDL_SHARED=OFF ^
    -DSDL_STATIC=ON ^
    -DSDL_TEST_LIBRARY=OFF ^
    -DSDL_TESTS=OFF ^
    -DSDL_EXAMPLES=OFF ^
    -DCMAKE_MSVC_RUNTIME_LIBRARY="MultiThreaded$<$<CONFIG:Debug>:Debug>" ^
    -DCMAKE_INSTALL_PREFIX="../install"

if !ERRORLEVEL! NEQ 0 (
    echo Failed to configure SDL3 build!
    cd "..\..\..\.."
    popd
    pause
    exit /b 1
)

echo Building SDL3 Debug configuration...
cmake --build . --config Debug

if !ERRORLEVEL! NEQ 0 (
    echo Failed to build SDL3 Debug!
    cd "..\..\..\.."
    popd
    pause
    exit /b 1
)

echo Building SDL3 Release configuration...
cmake --build . --config Release

if !ERRORLEVEL! NEQ 0 (
    echo Failed to build SDL3 Release!
    cd "..\..\..\.."
    popd
    pause
    exit /b 1
)

echo Installing SDL3 (Debug -^> install/Debug, Release -^> install/Release)...
cmake --install . --config Debug --prefix "../install/Debug"
cmake --install . --config Release --prefix "../install/Release"

cd "..\..\..\.."
echo SDL3 built and installed successfully!
goto :sdl3_done

:sdl3_rebuild
echo Rebuilding SDL3...
if not exist "Engine\Vendor\SDL3\build" (
    echo Build directory not found. Running full build instead...
    goto :sdl3_do_build
)
cd "Engine\Vendor\SDL3\build"
cmake --build . --config Debug
cmake --build . --config Release
cmake --install . --config Debug --prefix "../install/Debug"
cmake --install . --config Release --prefix "../install/Release"
cd "..\..\..\.."
echo SDL3 rebuilt successfully!

:sdl3_done

:: ============================================================
:: Setup GLM (OpenGL Mathematics)
:: ============================================================
echo.
echo Setting up GLM...
if not exist "Engine\Vendor\glm" (
    echo Cloning GLM...
    git clone --depth 1 https://github.com/g-truc/glm.git "Engine\Vendor\glm"
    
    if !ERRORLEVEL! NEQ 0 (
        echo Failed to clone GLM! Make sure git is installed.
        popd
        pause
        exit /b 1
    )
    
    echo GLM cloned successfully!
) else (
    if not defined NONINTERACTIVE (
        set /p UPDATE_GLM="GLM already exists. Update? (y/N): "
        if /i "!UPDATE_GLM!"=="y" (
            echo Updating GLM...
            cd "Engine\Vendor\glm"
            git pull origin master
            cd "..\..\.."
            echo GLM updated successfully!
        ) else (
            echo Skipping GLM update.
        )
    ) else (
        echo GLM already exists. Skipping.
    )
)

:: ============================================================
:: Setup doctest (header-only unit testing framework)
:: ============================================================
echo.
echo Setting up doctest...
if not exist "Engine\Vendor\doctest" (
    echo Cloning doctest...
    git clone --depth 1 https://github.com/doctest/doctest.git "Engine\Vendor\doctest"
    
    if !ERRORLEVEL! NEQ 0 (
        echo Failed to clone doctest! Make sure git is installed.
        popd
        pause
        exit /b 1
    )
    
    echo doctest cloned successfully!
) else (
    if not defined NONINTERACTIVE (
        set /p UPDATE_DOCTEST="doctest already exists. Update? (y/N): "
        if /i "!UPDATE_DOCTEST!"=="y" (
            echo Updating doctest...
            cd "Engine\Vendor\doctest"
            git pull origin master
            cd "..\..\.."
            echo doctest updated successfully!
        ) else (
            echo Skipping doctest update.
        )
    ) else (
        echo doctest already exists. Skipping.
    )
)

:: ============================================================
:: Setup stb (header-only for image loading)
:: ============================================================
echo.
echo Setting up stb...
if not exist "Engine\Vendor\stb" (
    mkdir "Engine\Vendor\stb"
)
if not exist "Engine\Vendor\stb\stb_image.h" (
    echo Downloading stb_image.h...
    powershell -Command "try { Invoke-WebRequest -Uri 'https://raw.githubusercontent.com/nothings/stb/master/stb_image.h' -OutFile 'Engine\Vendor\stb\stb_image.h' } catch { Write-Host 'Failed to download stb_image.h' }"
)
if not exist "Engine\Vendor\stb\stb_image_write.h" (
    echo Downloading stb_image_write.h...
    powershell -Command "try { Invoke-WebRequest -Uri 'https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h' -OutFile 'Engine\Vendor\stb\stb_image_write.h' } catch { Write-Host 'Failed to download stb_image_write.h' }"
)
if not exist "Engine\Vendor\stb\stb_image.cpp" (
    echo Creating stb_image.cpp TU...
    (echo #define STB_IMAGE_IMPLEMENTATION) > "Engine\Vendor\stb\stb_image.cpp"
    (echo #include "stb_image.h") >> "Engine\Vendor\stb\stb_image.cpp"
)
if not exist "Engine\Vendor\stb\stb_image_write.cpp" (
    echo Creating stb_image_write.cpp TU...
    (echo #define STB_IMAGE_WRITE_IMPLEMENTATION) > "Engine\Vendor\stb\stb_image_write.cpp"
    (echo #include "stb_image_write.h") >> "Engine\Vendor\stb\stb_image_write.cpp"
)

:: ============================================================
:: Setup nlohmann/json (JSON Library)
:: ============================================================
echo.
echo Setting up nlohmann/json...
if not exist "Engine\Vendor\nlohmann_json" (
    echo Cloning nlohmann/json...
    git clone --depth 1 --branch develop https://github.com/nlohmann/json.git "Engine\Vendor\nlohmann_json"
    
    if !ERRORLEVEL! NEQ 0 (
        echo Failed to clone nlohmann/json! Make sure git is installed.
        popd
        pause
        exit /b 1
    )
    
    echo nlohmann/json cloned successfully!
) else (
    if not defined NONINTERACTIVE (
        set /p UPDATE_JSON="nlohmann/json already exists. Update? (y/N): "
        if /i "!UPDATE_JSON!"=="y" (
            echo Updating nlohmann/json...
            cd "Engine\Vendor\nlohmann_json"
            git pull origin develop
            cd "..\..\.."
            echo nlohmann/json updated successfully!
        ) else (
            echo Skipping nlohmann/json update.
        )
    ) else (
        echo nlohmann/json already exists. Skipping.
    )
)

:: ============================================================
:: Setup Vulkan SDK
:: ============================================================
echo.
echo Setting up Vulkan SDK...
set VULKAN_SDK_VERSION=1.3.280.0
set VULKAN_FOUND=0

:: Check if system-wide Vulkan SDK is installed via environment variable
if defined VULKAN_SDK (
    if exist %VULKAN_SDK%"\Bin\vulkaninfoSDK.exe" (
        if exist %VULKAN_SDK%"\Lib\vulkan-1.lib" (
            echo Vulkan SDK found via system installation!
            echo Using: %VULKAN_SDK%
            goto :vulkan_found
        )
    )
)

:: If no system installation found, set up local version
if %VULKAN_FOUND% EQU 0 (
    echo No system Vulkan SDK found. Setting up local installation...
    
    :: Check if we already have a local installation with headers
    if exist "Engine\Vendor\VulkanSDK\Include\vulkan\vulkan.h" (
        echo Local Vulkan SDK headers found.
        set "VULKAN_SDK=%CD%\Engine\Vendor\VulkanSDK"
        goto :vulkan_found
    )
    
    if not exist "Engine\Vendor\VulkanSDK" mkdir "Engine\Vendor\VulkanSDK"
    
    :: Skip full SDK download for faster setup - just use headers-only approach
    echo Skipping full SDK download to avoid long wait times...
    echo Note: You can manually install the full Vulkan SDK later if needed.
    echo Download from: https://vulkan.lunarg.com/sdk/download/latest/windows/vulkan-sdk.exe
    
    :: Always set up minimal headers for compilation (even if installer downloaded)
    echo Setting up minimal Vulkan headers for development...
    
    :: Create basic directory structure
    if not exist "Engine\Vendor\VulkanSDK\Include\vulkan" mkdir "Engine\Vendor\VulkanSDK\Include\vulkan"

    :: Download essential headers
    echo Downloading essential Vulkan headers...
    powershell -Command "try { Invoke-WebRequest -Uri 'https://raw.githubusercontent.com/KhronosGroup/Vulkan-Headers/main/include/vulkan/vulkan.h' -OutFile 'Engine\Vendor\VulkanSDK\Include\vulkan\vulkan.h' } catch { Write-Host 'Failed to download vulkan.h' }"
    powershell -Command "try { Invoke-WebRequest -Uri 'https://raw.githubusercontent.com/KhronosGroup/Vulkan-Headers/main/include/vulkan/vulkan_core.h' -OutFile 'Engine\Vendor\VulkanSDK\Include\vulkan\vulkan_core.h' } catch { Write-Host 'Failed to download vulkan_core.h' }"
    powershell -Command "try { Invoke-WebRequest -Uri 'https://raw.githubusercontent.com/KhronosGroup/Vulkan-Headers/main/include/vulkan/vk_platform.h' -OutFile 'Engine\Vendor\VulkanSDK\Include\vulkan\vk_platform.h' } catch { Write-Host 'Failed to download vk_platform.h' }"
    powershell -Command "try { Invoke-WebRequest -Uri 'https://raw.githubusercontent.com/KhronosGroup/Vulkan-Headers/main/include/vulkan/vulkan_win32.h' -OutFile 'Engine\Vendor\VulkanSDK\Include\vulkan\vulkan_win32.h' } catch { Write-Host 'Failed to download vulkan_win32.h' }"
    
    :: Check if headers were downloaded successfully
    if exist "Engine\Vendor\VulkanSDK\Include\vulkan\vulkan.h" (
        echo Vulkan headers downloaded successfully!
    ) else (
        echo Failed to download Vulkan headers. You may need to install the full Vulkan SDK manually.
        echo Download from: https://vulkan.lunarg.com/sdk/download/latest/windows/vulkan-sdk.exe
    )
    
    :: Note about Vulkan library - will use headers-only approach
    echo.
    echo Note: Using headers-only Vulkan setup. The project will compile and can use
    echo dynamic loading at runtime. For full development features, consider installing
    echo the complete Vulkan SDK from the official website.
    echo.
)

:skip_vulkan_installer

:: Set local Vulkan SDK path for this session if using local installation
if exist "Engine\Vendor\VulkanSDK\Include" (
    if not defined VULKAN_SDK (
        set "VULKAN_SDK=%CD%\Engine\Vendor\VulkanSDK"
        echo Local Vulkan SDK path set: !VULKAN_SDK!
    )
)

:vulkan_found
echo Vulkan SDK setup complete.
if defined VULKAN_SDK (
    echo Using Vulkan SDK: %VULKAN_SDK%
    if exist "%VULKAN_SDK%\Lib\vulkan-1.lib" (
        echo Vulkan library found: %VULKAN_SDK%\Lib\vulkan-1.lib
    )
)

:: ============================================================
:: Setup SPIRV-Headers (dependency for shaderc)
:: ============================================================
echo.
echo Setting up SPIRV-Headers...
if not exist "Engine\Vendor\SPIRV-Headers" (
    echo Cloning SPIRV-Headers...
    git clone --depth 1 https://github.com/KhronosGroup/SPIRV-Headers.git "Engine\Vendor\SPIRV-Headers"
    
    if !ERRORLEVEL! NEQ 0 (
        echo Failed to clone SPIRV-Headers! Make sure git is installed.
        popd
        pause
        exit /b 1
    )
    
    echo SPIRV-Headers cloned successfully!
) else (
    if not defined NONINTERACTIVE (
        set /p UPDATE_SPIRVH="SPIRV-Headers already exists. Update? (y/N): "
        if /i "!UPDATE_SPIRVH!"=="y" (
            echo Updating SPIRV-Headers...
            cd "Engine\Vendor\SPIRV-Headers"
            git pull origin main
            cd "..\..\.."
            echo SPIRV-Headers updated successfully!
        ) else (
            echo Skipping SPIRV-Headers update.
        )
    ) else (
        echo SPIRV-Headers already exists. Skipping.
    )
)

:: ============================================================
:: Setup SPIRV-Tools (dependency for shaderc)
:: ============================================================
echo.
echo Setting up SPIRV-Tools...
set SPIRVTOOLS_NEEDS_BUILD=0
set SPIRVTOOLS_IS_NEW=0
if not exist "Engine\Vendor\SPIRV-Tools" (
    echo Cloning SPIRV-Tools...
    git clone --recurse-submodules https://github.com/KhronosGroup/SPIRV-Tools.git "Engine\Vendor\SPIRV-Tools"
    
    if !ERRORLEVEL! NEQ 0 (
        echo Failed to clone SPIRV-Tools! Make sure git is installed.
        popd
        pause
        exit /b 1
    )
    
    echo SPIRV-Tools cloned successfully!
    set SPIRVTOOLS_NEEDS_BUILD=1
    set SPIRVTOOLS_IS_NEW=1
) else (
    if not defined NONINTERACTIVE (
        set /p UPDATE_SPIRVT="SPIRV-Tools already exists. Update? (y/N): "
        if /i "!UPDATE_SPIRVT!"=="y" (
            echo Updating SPIRV-Tools...
            cd "Engine\Vendor\SPIRV-Tools"
            git pull origin main
            git submodule update --init --recursive
            cd "..\..\.."
            echo SPIRV-Tools updated successfully!
            set SPIRVTOOLS_NEEDS_BUILD=1
        ) else (
            echo Skipping SPIRV-Tools update.
        )
    ) else (
        echo SPIRV-Tools already exists. Skipping.
    )
)

:: Check if SPIRV-Tools is built
set SPIRVTOOLS_BUILT=0
if exist "Engine\Vendor\SPIRV-Tools\install\Debug\lib\SPIRV-Tools.lib" set SPIRVTOOLS_BUILT=1
if exist "Engine\Vendor\SPIRV-Tools\install\Release\lib\SPIRV-Tools.lib" set SPIRVTOOLS_BUILT=1
if exist "Engine\Vendor\SPIRV-Tools\install\lib\SPIRV-Tools.lib" set SPIRVTOOLS_BUILT=1

:: If just cloned or updated, build it
if !SPIRVTOOLS_IS_NEW! EQU 1 goto :spirvtools_do_build
if !SPIRVTOOLS_NEEDS_BUILD! EQU 1 goto :spirvtools_do_build

:: If not built, ask user
if !SPIRVTOOLS_BUILT! EQU 0 (
    if not defined NONINTERACTIVE (
        set /p BUILD_SPIRVT="SPIRV-Tools is not built. Build now? (y/N): "
        if /i "!BUILD_SPIRVT!"=="y" (
            goto :spirvtools_do_build
        ) else (
            echo Skipping SPIRV-Tools build. Note: Shader compilation may fail!
            goto :spirvtools_done
        )
    ) else (
        echo SPIRV-Tools not built. Building...
        goto :spirvtools_do_build
    )
) else (
    if not defined NONINTERACTIVE (
        set /p REBUILD_SPIRVT="SPIRV-Tools is already built. Rebuild? (y/N): "
        if /i "!REBUILD_SPIRVT!"=="y" (
            goto :spirvtools_rebuild
        ) else (
            echo Skipping SPIRV-Tools rebuild.
            goto :spirvtools_done
        )
    ) else (
        echo SPIRV-Tools already built. Skipping.
        goto :spirvtools_done
    )
)

:spirvtools_do_build
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
if !ERRORLEVEL! GEQ 8 (
    echo Failed to copy SPIRV-Headers!
    cd "..\..\.."
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

if !ERRORLEVEL! NEQ 0 (
    echo Failed to configure SPIRV-Tools build!
    cd "..\..\..\.."
    popd
    pause
    exit /b 1
)

echo Building SPIRV-Tools Debug configuration...
cmake --build . --config Debug

if !ERRORLEVEL! NEQ 0 (
    echo Failed to build SPIRV-Tools Debug!
    cd "..\..\..\.."
    popd
    pause
    exit /b 1
)

echo Building SPIRV-Tools Release configuration...
cmake --build . --config Release

if !ERRORLEVEL! NEQ 0 (
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
goto :spirvtools_done

:spirvtools_rebuild
echo Rebuilding SPIRV-Tools...
if not exist "Engine\Vendor\SPIRV-Tools\build" (
    echo Build directory not found. Running full build instead...
    goto :spirvtools_do_build
)
cd "Engine\Vendor\SPIRV-Tools\build"
cmake --build . --config Debug
cmake --build . --config Release
cmake --install . --config Debug --prefix "../install/Debug"
cmake --install . --config Release --prefix "../install/Release"
cd "..\..\..\.."
echo SPIRV-Tools rebuilt successfully!

:spirvtools_done

:: ============================================================
:: Setup shaderc (Google's shader compiler)
:: ============================================================
echo.
echo Setting up shaderc...
set SHADERC_NEEDS_BUILD=0
set SHADERC_IS_NEW=0
if not exist "Engine\Vendor\shaderc" (
    echo Cloning shaderc...
    git clone https://github.com/google/shaderc.git "Engine\Vendor\shaderc"
    
    if !ERRORLEVEL! NEQ 0 (
        echo Failed to clone shaderc! Make sure git is installed.
        popd
        pause
        exit /b 1
    )
    
    echo shaderc cloned successfully!
    set SHADERC_NEEDS_BUILD=1
    set SHADERC_IS_NEW=1
) else (
    if not defined NONINTERACTIVE (
        set /p UPDATE_SHADERC="shaderc already exists. Update? (y/N): "
        if /i "!UPDATE_SHADERC!"=="y" (
            echo Updating shaderc...
            cd "Engine\Vendor\shaderc"
            git pull origin main
            cd "..\..\.."
            echo shaderc updated successfully!
            set SHADERC_NEEDS_BUILD=1
        ) else (
            echo Skipping shaderc update.
        )
    ) else (
        echo shaderc already exists. Skipping.
    )
)

:: Check if shaderc is built
set SHADERC_BUILT=0
if exist "Engine\Vendor\shaderc\install\Debug\lib\shaderc.lib" set SHADERC_BUILT=1
if exist "Engine\Vendor\shaderc\install\Release\lib\shaderc.lib" set SHADERC_BUILT=1
if exist "Engine\Vendor\shaderc\install\lib\shaderc.lib" set SHADERC_BUILT=1
if exist "Engine\Vendor\shaderc\install\lib\shaderc_combined.lib" set SHADERC_BUILT=1

:: If just cloned or updated, build it
if !SHADERC_IS_NEW! EQU 1 goto :shaderc_do_build
if !SHADERC_NEEDS_BUILD! EQU 1 goto :shaderc_do_build

:: If not built, ask user
if !SHADERC_BUILT! EQU 0 (
    if not defined NONINTERACTIVE (
        set /p BUILD_SHADERC="shaderc is not built. Build now? (y/N): "
        if /i "!BUILD_SHADERC!"=="y" (
            goto :shaderc_do_build
        ) else (
            echo Skipping shaderc build. Note: Shader compilation may fail!
            goto :shaderc_done
        )
    ) else (
        echo shaderc not built. Building...
        goto :shaderc_do_build
    )
) else (
    if not defined NONINTERACTIVE (
        set /p REBUILD_SHADERC="shaderc is already built. Rebuild? (y/N): "
        if /i "!REBUILD_SHADERC!"=="y" (
            goto :shaderc_rebuild
        ) else (
            echo Skipping shaderc rebuild.
            goto :shaderc_done
        )
    ) else (
        echo shaderc already built. Skipping.
        goto :shaderc_done
    )
)

:shaderc_do_build
:: Setup shaderc dependencies using git-sync-deps
echo Setting up shaderc dependencies...
cd "Engine\Vendor\shaderc"

:: Ensure the git-sync-deps script exists
if not exist "utils\git-sync-deps" (
    echo [ERROR] shaderc dependency script not found at utils\git-sync-deps
    cd "..\..\.."
    popd
    pause
    exit /b 1
)

:: Ensure we have a Python command available
if not defined PYTHON_CMD (
    echo WARNING: PYTHON_CMD not set. Attempting to locate Python...
    for %%P in (py python python3) do (
        %%P --version >nul 2>&1 && set "PYTHON_CMD=%%P" && goto :shaderc_have_python
    )
    echo [ERROR] Python interpreter not found to run git-sync-deps.
    cd "..\..\.."
    popd
    pause
    exit /b 1
)

:shaderc_have_python
echo Running git-sync-deps to fetch shaderc dependencies using: %PYTHON_CMD%
"%PYTHON_CMD%" "utils\git-sync-deps"

if !ERRORLEVEL! NEQ 0 (
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

if !ERRORLEVEL! NEQ 0 (
    echo Failed to configure shaderc build!
    cd "..\..\..\.."
    popd
    pause
    exit /b 1
)

echo Building shaderc Debug configuration...
cmake --build . --config Debug

if !ERRORLEVEL! NEQ 0 (
    echo Failed to build shaderc Debug!
    cd "..\..\..\.."
    popd
    pause
    exit /b 1
)

echo Building shaderc Release configuration...
cmake --build . --config Release

if !ERRORLEVEL! NEQ 0 (
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
goto :shaderc_done

:shaderc_rebuild
echo Rebuilding shaderc...
if not exist "Engine\Vendor\shaderc\build" (
    echo Build directory not found. Running full build instead...
    goto :shaderc_do_build
)
cd "Engine\Vendor\shaderc\build"
cmake --build . --config Debug
cmake --build . --config Release
cmake --install . --config Debug --prefix "../install/Debug"
cmake --install . --config Release --prefix "../install/Release"
cd "..\..\..\.."
echo shaderc rebuilt successfully!

:shaderc_done

:: ============================================================
:: Setup SPIRV-Cross (for shader reflection and cross-compilation)
:: ============================================================
echo.
echo Setting up SPIRV-Cross...
set SPIRVCROSS_NEEDS_BUILD=0
set SPIRVCROSS_IS_NEW=0
if not exist "Engine\Vendor\SPIRV-Cross" (
    echo Cloning SPIRV-Cross...
    git clone https://github.com/KhronosGroup/SPIRV-Cross.git "Engine\Vendor\SPIRV-Cross"
    
    if !ERRORLEVEL! NEQ 0 (
        echo Failed to clone SPIRV-Cross! Make sure git is installed.
        popd
        pause
        exit /b 1
    )
    
    echo SPIRV-Cross cloned successfully!
    set SPIRVCROSS_NEEDS_BUILD=1
    set SPIRVCROSS_IS_NEW=1
) else (
    if not defined NONINTERACTIVE (
        set /p UPDATE_SPIRVC="SPIRV-Cross already exists. Update? (y/N): "
        if /i "!UPDATE_SPIRVC!"=="y" (
            echo Updating SPIRV-Cross...
            cd "Engine\Vendor\SPIRV-Cross"
            git pull origin main
            cd "..\..\.."
            echo SPIRV-Cross updated successfully!
            set SPIRVCROSS_NEEDS_BUILD=1
        ) else (
            echo Skipping SPIRV-Cross update.
        )
    ) else (
        echo SPIRV-Cross already exists. Skipping.
    )
)

:: Check if SPIRV-Cross is built
set SPIRVCROSS_BUILT=0
if exist "Engine\Vendor\SPIRV-Cross\install\Debug\lib\spirv-cross-core.lib" set SPIRVCROSS_BUILT=1
if exist "Engine\Vendor\SPIRV-Cross\install\Release\lib\spirv-cross-core.lib" set SPIRVCROSS_BUILT=1
if exist "Engine\Vendor\SPIRV-Cross\install\lib\spirv-cross-core.lib" set SPIRVCROSS_BUILT=1

:: If just cloned or updated, build it
if !SPIRVCROSS_IS_NEW! EQU 1 goto :spirvcross_do_build
if !SPIRVCROSS_NEEDS_BUILD! EQU 1 goto :spirvcross_do_build

:: If not built, ask user
if !SPIRVCROSS_BUILT! EQU 0 (
    if not defined NONINTERACTIVE (
        set /p BUILD_SPIRVC="SPIRV-Cross is not built. Build now? (y/N): "
        if /i "!BUILD_SPIRVC!"=="y" (
            goto :spirvcross_do_build
        ) else (
            echo Skipping SPIRV-Cross build. Note: Shader reflection may fail!
            goto :spirvcross_done
        )
    ) else (
        echo SPIRV-Cross not built. Building...
        goto :spirvcross_do_build
    )
) else (
    if not defined NONINTERACTIVE (
        set /p REBUILD_SPIRVC="SPIRV-Cross is already built. Rebuild? (y/N): "
        if /i "!REBUILD_SPIRVC!"=="y" (
            goto :spirvcross_rebuild
        ) else (
            echo Skipping SPIRV-Cross rebuild.
            goto :spirvcross_done
        )
    ) else (
        echo SPIRV-Cross already built. Skipping.
        goto :spirvcross_done
    )
)

:spirvcross_do_build
echo Building SPIRV-Cross...
if not exist "Engine\Vendor\SPIRV-Cross\build" mkdir "Engine\Vendor\SPIRV-Cross\build"
cd "Engine\Vendor\SPIRV-Cross\build"

echo Configuring SPIRV-Cross build...
cmake .. -G "Visual Studio 17 2022" -A x64 ^
    -DSPIRV_CROSS_ENABLE_TESTS=OFF ^
    -DSPIRV_CROSS_ENABLE_GLSL=ON ^
    -DSPIRV_CROSS_ENABLE_HLSL=ON ^
    -DSPIRV_CROSS_ENABLE_MSL=OFF ^
    -DSPIRV_CROSS_ENABLE_CPP=OFF ^
    -DSPIRV_CROSS_ENABLE_REFLECT=OFF ^
    -DSPIRV_CROSS_ENABLE_C_API=ON ^
    -DSPIRV_CROSS_ENABLE_UTIL=ON ^
    -DSPIRV_CROSS_CLI=OFF ^
    -DBUILD_SHARED_LIBS=OFF ^
    -DCMAKE_MSVC_RUNTIME_LIBRARY="MultiThreaded$<$<CONFIG:Debug>:Debug>" ^
    -DCMAKE_CXX_FLAGS_DEBUG="/MTd" ^
    -DCMAKE_CXX_FLAGS_RELEASE="/MT" ^
    -DCMAKE_C_FLAGS_DEBUG="/MTd" ^
    -DCMAKE_C_FLAGS_RELEASE="/MT" ^
    -DCMAKE_INSTALL_PREFIX="../install"

if !ERRORLEVEL! NEQ 0 (
    echo Failed to configure SPIRV-Cross build!
    cd "..\..\..\.."
    popd
    pause
    exit /b 1
)

echo Building SPIRV-Cross Debug configuration...
cmake --build . --config Debug

if !ERRORLEVEL! NEQ 0 (
    echo Failed to build SPIRV-Cross Debug!
    cd "..\..\..\.."
    popd
    pause
    exit /b 1
)

echo Building SPIRV-Cross Release configuration...
cmake --build . --config Release

if !ERRORLEVEL! NEQ 0 (
    echo Failed to build SPIRV-Cross Release!
    cd "..\..\..\.."
    popd
    pause
    exit /b 1
)

echo Installing SPIRV-Cross...
cmake --install . --config Debug --prefix "../install/Debug"
cmake --install . --config Release --prefix "../install/Release"

cd "..\..\..\.."
echo SPIRV-Cross built and installed successfully!
goto :spirvcross_done

:spirvcross_rebuild
echo Rebuilding SPIRV-Cross...
if not exist "Engine\Vendor\SPIRV-Cross\build" (
    echo Build directory not found. Running full build instead...
    goto :spirvcross_do_build
)
cd "Engine\Vendor\SPIRV-Cross\build"
cmake --build . --config Debug
cmake --build . --config Release
cmake --install . --config Debug --prefix "../install/Debug"
cmake --install . --config Release --prefix "../install/Release"
cd "..\..\..\.."
echo SPIRV-Cross rebuilt successfully!

:spirvcross_done

:: ============================================================
:: Setup GLAD
:: ============================================================
echo.
echo Setting up GLAD...
set GLAD_NEEDS_GENERATE=0
set GLAD_IS_NEW=0
if not exist "Engine\Vendor\GLAD" (
    echo Cloning GLAD...
    git clone --depth 1 https://github.com/Dav1dde/glad.git "Engine\Vendor\GLAD"
    
    if !ERRORLEVEL! NEQ 0 (
        echo Failed to clone GLAD! Make sure git is installed.
        popd
        pause
        exit /b 1
    )
    
    echo GLAD cloned successfully!
    set GLAD_NEEDS_GENERATE=1
    set GLAD_IS_NEW=1
) else (
    if not defined NONINTERACTIVE (
        set /p UPDATE_GLAD="GLAD already exists. Update? (y/N): "
        if /i "!UPDATE_GLAD!"=="y" (
            echo Updating GLAD...
            cd "Engine\Vendor\GLAD"
            git pull origin master
            cd "..\..\.."
            echo GLAD updated successfully!
            set GLAD_NEEDS_GENERATE=1
        ) else (
            echo Skipping GLAD update.
        )
    ) else (
        echo GLAD already exists. Skipping.
    )
)

:: Check if GLAD files are generated
set GLAD_GENERATED=0
if exist "Engine\Vendor\GLAD\generated\include\glad\gl.h" (
    if exist "Engine\Vendor\GLAD\generated\src\gl.c" (
        set GLAD_GENERATED=1
    )
)

:: If just cloned or updated, generate
if !GLAD_IS_NEW! EQU 1 goto :glad_do_generate
if !GLAD_NEEDS_GENERATE! EQU 1 goto :glad_do_generate

:: If not generated, ask user
if !GLAD_GENERATED! EQU 0 (
    if not defined NONINTERACTIVE (
        set /p GEN_GLAD="GLAD files not generated. Generate now? (y/N): "
        if /i "!GEN_GLAD!"=="y" (
            goto :glad_do_generate
        ) else (
            echo Skipping GLAD generation. Note: Build may fail without GLAD!
            goto :glad_done
        )
    ) else (
        echo GLAD not generated. Generating...
        goto :glad_do_generate
    )
) else (
    echo GLAD files already generated. Skipping.
    goto :glad_done
)

:glad_do_generate
echo Generating GLAD files for OpenGL 4.6 Core Profile...
cd "Engine\Vendor\GLAD"

:: Create generated directory if it doesn't exist
if not exist "generated" mkdir "generated"

:: Generate OpenGL 4.6 Core Profile with extensions
echo Using Python command: %PYTHON_CMD%
echo Generating GLAD for OpenGL 4.6 Core...

%PYTHON_CMD% -m glad --api="gl:core=4.6" --out-path=generated c
set GLAD_ERROR=!ERRORLEVEL!

if !GLAD_ERROR! NEQ 0 (
    echo Failed to generate GLAD files!
    echo Installing required Python packages...
    
    :: Try different pip commands
    set PIP_FOUND=0
    
    :: Try pip
    pip --version >nul 2>&1
    if !ERRORLEVEL! EQU 0 (
        set PIP_FOUND=1
        set "PIP_CMD=pip"
    ) else (
        :: Try python -m pip
        %PYTHON_CMD% -m pip --version >nul 2>&1
        if !ERRORLEVEL! EQU 0 (
            set PIP_FOUND=1
            set "PIP_CMD=%PYTHON_CMD% -m pip"
        )
    )
    
    if !PIP_FOUND! EQU 0 (
        echo pip not found! Please install pip or use a Python installation that includes pip.
        cd "..\..\.."
        popd
        pause
        exit /b 1
    )
    
    echo Using pip command: !PIP_CMD!
    echo Installing jinja2 and glad2...
    !PIP_CMD! install jinja2 glad2 --user
    set PIP_ERROR=!ERRORLEVEL!
    
    if !PIP_ERROR! NEQ 0 (
        echo Failed to install required packages!
        cd "..\..\.."
        popd
        pause
        exit /b 1
    )
    
    echo Required packages installed, retrying generation...
    %PYTHON_CMD% -m glad --api="gl:core=4.6" --out-path=generated c
    set GLAD_RETRY_ERROR=!ERRORLEVEL!
    
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

:glad_done

:: ============================================================
:: Setup ImGui with docking branch
:: ============================================================
echo.
echo Setting up ImGui with docking support...
if not exist "Engine\Vendor\imgui" (
    echo Cloning ImGui docking branch...
    git clone --depth 1 --branch docking https://github.com/ocornut/imgui.git "Engine\Vendor\imgui"
    
    if !ERRORLEVEL! NEQ 0 (
        echo Failed to clone ImGui! Make sure git is installed.
        popd
        pause
        exit /b 1
    )
    
    echo ImGui docking branch cloned successfully!
) else (
    if not defined NONINTERACTIVE (
        set /p UPDATE_IMGUI="ImGui already exists. Update? (y/N): "
        if /i "!UPDATE_IMGUI!"=="y" (
            echo Updating ImGui docking branch...
            cd "Engine\Vendor\imgui"
            git checkout docking
            git pull origin docking
            cd "..\..\.."
            echo ImGui docking branch updated successfully!
        ) else (
            echo Skipping ImGui update.
        )
    ) else (
        echo ImGui already exists. Skipping.
    )
)

:: ============================================================
:: Generate Project Files
:: ============================================================
echo.
echo Generating Visual Studio project files...
call "Vendor\Premake\premake5.exe" vs2022

if %ERRORLEVEL% NEQ 0 (
    echo Failed to generate project files!
    popd
    pause
    exit /b 1
)

echo.
echo ============================================================
echo                   Setup Complete!
echo ============================================================
echo Project files generated successfully!
echo You can now open Vortex.sln in Visual Studio 2022
echo.

:: Return to original directory
popd
if not defined NONINTERACTIVE pause
