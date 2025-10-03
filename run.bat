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

cmake .. -G "Visual Studio 17 2022" -A x64
if %errorlevel% neq 0 (
    echo ERROR: CMake configuration failed
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