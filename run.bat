@echo off
REM =====================================================
REM CMake Build Script for Lets-Eat-Monsters Project
REM Builds both Debug and Release configurations
REM =====================================================

echo Starting CMake build process...
echo.

REM Check if CMake is available
cmake --version >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: CMake is not installed or not in PATH
    echo Please install CMake and add it to your system PATH
    pause
    exit /b 1
)

REM Set build directory
set BUILD_DIR=build

REM Create build directory if it doesn't exist
if not exist "%BUILD_DIR%" (
    echo Creating build directory...
    mkdir "%BUILD_DIR%"
)

REM Navigate to build directory
cd "%BUILD_DIR%"

REM =====================================================
REM Configure CMake (imports dependencies automatically)
REM =====================================================
echo Configuring CMake project and importing dependencies...
echo This may take a while for the first run as dependencies are downloaded...
echo.

REM Auto-detect Visual Studio version using vswhere
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set VS_GENERATOR=

if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -property catalog_productLineVersion 2^>nul`) do set VS_VERSION=%%i
)

if "%VS_VERSION%"=="2022" (
    set "VS_GENERATOR=Visual Studio 17 2022"
    goto :run_cmake
)
if "%VS_VERSION%"=="2019" (
    set "VS_GENERATOR=Visual Studio 16 2019"
    goto :run_cmake
)

REM Fallback: let CMake auto-detect
echo Could not detect Visual Studio version, letting CMake choose...
set VS_GENERATOR=

:run_cmake
if "%VS_GENERATOR%"=="" (
    cmake .. -A x64
) else (
    echo Using generator: %VS_GENERATOR%
    cmake .. -G "%VS_GENERATOR%" -A x64
)

if %errorlevel% neq 0 (
    echo.
    echo ERROR: CMake configuration failed
    echo.
    echo Make sure you have Visual Studio installed with C++ development tools.
    echo Supported versions: Visual Studio 2019 or 2022
    cd ..
    pause
    exit /b 1
)

echo CMake configuration completed successfully!
echo.

REM =====================================================
REM Build Debug Configuration
REM =====================================================
echo Building Debug configuration...
cmake --build . --config Debug
if %errorlevel% neq 0 (
    echo ERROR: Debug build failed
    cd ..
    pause
    exit /b 1
)

echo Debug build completed successfully!
echo.

REM =====================================================
REM Build Release Configuration
REM =====================================================
echo Building Release configuration...
cmake --build . --config Release
if %errorlevel% neq 0 (
    echo ERROR: Release build failed
    cd ..
    pause
    exit /b 1
)

echo Release build completed successfully!
echo.

REM Navigate back to project root
cd ..

REM =====================================================
REM Build Summary
REM =====================================================
echo =====================================================
echo BUILD SUMMARY
echo =====================================================
echo Debug executable:   %BUILD_DIR%\Debug\LetsEatMonsters.exe
echo Release executable: %BUILD_DIR%\Release\LetsEatMonsters.exe
echo.
echo Both configurations built successfully!
echo.

REM Check if executables exist
if exist "%BUILD_DIR%\Debug\LetsEatMonsters.exe" (
    echo Debug executable created
) else (
    echo Debug executable not found
)

if exist "%BUILD_DIR%\Release\LetsEatMonsters.exe" (
    echo Release executable created  
) else (
    echo Release executable not found
)

echo.
echo Build process completed!
pause