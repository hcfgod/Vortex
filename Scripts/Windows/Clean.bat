@echo off
echo Cleaning Vortex Engine...

:: Navigate to project root (go up two directories from Scripts\Windows)
pushd "%~dp0..\.."

echo Removing build files...
if exist "Build\" (
    rmdir /s /q "Build\"
    echo Removed Build directory
)

echo Removing Visual Studio files...
if exist "*.sln" del "*.sln"
if exist "*.vcxproj" del "*.vcxproj"
if exist "*.vcxproj.filters" del "*.vcxproj.filters"
if exist "*.vcxproj.user" del "*.vcxproj.user"

echo Clean completed!

:: Return to original directory
popd
pause
