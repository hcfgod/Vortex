@echo off
echo Building Vortex Engine...

:: Navigate to project root (go up two directories from Scripts\Windows)
pushd "%~dp0..\.."

:: Find MSBuild location
set MSBUILD_PATH=""

:: Try VS 2022 Community
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" (
    set MSBUILD_PATH="C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
    goto :found_msbuild
)

:: Try VS 2022 Professional
if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe" (
    set MSBUILD_PATH="C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe"
    goto :found_msbuild
)

:: Try VS 2022 Enterprise
if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe" (
    set MSBUILD_PATH="C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe"
    goto :found_msbuild
)

:: Try VS 2019 Community
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe" (
    set MSBUILD_PATH="C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe"
    goto :found_msbuild
)

:: Try VS 2019 Professional
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\MSBuild\Current\Bin\MSBuild.exe" (
    set MSBUILD_PATH="C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\MSBuild\Current\Bin\MSBuild.exe"
    goto :found_msbuild
)

:: Try Build Tools
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\MSBuild\Current\Bin\MSBuild.exe" (
    set MSBUILD_PATH="C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\MSBuild\Current\Bin\MSBuild.exe"
    goto :found_msbuild
)

:: Try using vswhere to find MSBuild
for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.Component.MSBuild -property installationPath`) do (
    if exist "%%i\MSBuild\Current\Bin\MSBuild.exe" (
        set MSBUILD_PATH="%%i\MSBuild\Current\Bin\MSBuild.exe"
        goto :found_msbuild
    )
)

echo MSBuild not found! Please make sure Visual Studio or Build Tools are installed.
echo Searched common locations but couldn't find MSBuild.exe
popd
pause
exit /b 1

:found_msbuild
echo Found MSBuild at: %MSBUILD_PATH%

if not exist "Vortex.sln" (
    echo No solution file found! Running setup first...
    call "Scripts\Windows\Setup_Complete.bat"
    if %ERRORLEVEL% NEQ 0 (
        echo Setup failed!
        popd
        pause
        exit /b 1
    )
)

echo Building Debug configuration...
%MSBUILD_PATH% Vortex.sln /p:Configuration=Debug /p:Platform=x64 /m

if %ERRORLEVEL% NEQ 0 (
    echo Debug build failed!
    popd
    pause
    exit /b 1
)

echo Building Release configuration...
%MSBUILD_PATH% Vortex.sln /p:Configuration=Release /p:Platform=x64 /m

if %ERRORLEVEL% NEQ 0 (
    echo Release build failed!
    popd
    pause
    exit /b 1
)

echo Build completed successfully!

:: Return to original directory
popd
pause
