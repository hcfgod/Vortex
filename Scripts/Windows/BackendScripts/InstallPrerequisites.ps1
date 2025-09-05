<#
.SYNOPSIS
    Setup script for project prerequisites (Premake, Git, CMake, Python)
.DESCRIPTION
    - Ensures Premake5 is installed
    - Checks for Git, CMake, and Python
    - Guides user to install missing prerequisites
#>

# Navigate to project root (go up two directories from Scripts\Windows)
Set-Location -Path (Resolve-Path "$PSScriptRoot\..\..\..")

# Check if Premake5 exists, if not download it
$premakePath = "Vendor\Premake\premake5.exe"
if (-not (Test-Path $premakePath)) {
    Write-Host "Premake5 not found. Downloading..."
    if (-not (Test-Path "Vendor\Premake")) {
        New-Item -ItemType Directory -Path "Vendor\Premake" | Out-Null
    }

    Write-Host "Downloading Premake5 for Windows..."
    try {
        Invoke-WebRequest -Uri "https://github.com/premake/premake-core/releases/download/v5.0.0-beta2/premake-5.0.0-beta2-windows.zip" -OutFile "Vendor\Premake\premake.zip"
    } catch {
        Write-Host "Failed to download Premake5!" -ForegroundColor Red
        Pause
        exit 1
    }

    Write-Host "Extracting Premake5..."
    Expand-Archive -Path "Vendor\Premake\premake.zip" -DestinationPath "Vendor\Premake" -Force
    Remove-Item "Vendor\Premake\premake.zip"

    Write-Host "Premake5 installed successfully!"
}

# Check prerequisites
Write-Host "`nChecking prerequisites..."
$prerequisitesMissing = $false

# Check for Git
Write-Host "Checking for Git..."
if (Get-Command git -ErrorAction SilentlyContinue) {
    Write-Host "[OK] Git found"
} else {
    Write-Host "[ERROR] Git not found!" -ForegroundColor Red
    $prerequisitesMissing = $true
}

# Check for CMake
Write-Host "Checking for CMake..."
if (Get-Command cmake -ErrorAction SilentlyContinue) {
    Write-Host "[OK] CMake found"
} else {
    Write-Host "[ERROR] CMake not found!" -ForegroundColor Red
    $prerequisitesMissing = $true
}

# Check for Python
Write-Host "Checking for Python..."
$pythonCmd = $null

$pythonCandidates = @("py", "python", "python3")
foreach ($cmd in $pythonCandidates) {
    if (Get-Command $cmd -ErrorAction SilentlyContinue) {
        $pythonCmd = $cmd
        Write-Host "[OK] Python found via '$cmd'"
        break
    }
}

if (-not $pythonCmd) {
    # Try searching common installation paths
    $localPython = Join-Path $env:LOCALAPPDATA "Programs\Python"
    if (Test-Path $localPython) {
        $dirs = Get-ChildItem -Path $localPython -Directory -Filter "Python*"
        foreach ($dir in $dirs) {
            $exe = Join-Path $dir.FullName "python.exe"
            if (Test-Path $exe) {
                $pythonCmd = $exe
                Write-Host "[OK] Python found at $exe"
                break
            }
        }
    }
}

if (-not $pythonCmd) {
    Write-Host "[ERROR] Python not found!" -ForegroundColor Red
    $prerequisitesMissing = $true
}

# If any prerequisites missing, show instructions
if ($prerequisitesMissing) {
    Write-Host ""
    Write-Host "========================================"
    Write-Host "MISSING PREREQUISITES DETECTED"
    Write-Host "========================================"
    Write-Host ""
    Write-Host "Please install the following tools before running this setup script:`n"
    Write-Host "1. Git - https://git-scm.com/download/windows"
    Write-Host "   - Required for downloading dependencies`n"
    Write-Host "2. CMake - https://cmake.org/download/"
    Write-Host "   - Required for building C++ dependencies"
    Write-Host "   - Make sure to add CMake to your PATH during installation`n"
    Write-Host "3. Python - https://www.python.org/downloads/"
    Write-Host "   - Required for generating GLAD OpenGL loader"
    Write-Host "   - Make sure to check 'Add Python to PATH' during installation`n"
    Pause
    exit 1
}

Write-Host ""
Write-Host "All prerequisites found! Continuing with setup..."
Write-Host ""