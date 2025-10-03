@echo off
REM =====================================================
REM Clean Script for Lets-Eat-Monsters Project
REM Removes build artifacts and CMake cache files
REM =====================================================

echo Starting clean process...
echo.

REM Set build directory
set BUILD_DIR=build

REM Check if build directory exists
if not exist "%BUILD_DIR%" (
    echo Build directory '%BUILD_DIR%' does not exist.
    echo Nothing to clean.
    echo.
    pause
    exit /b 0
)

REM Confirm deletion
echo This will delete the entire build directory and all compiled files.
echo Directory to be deleted: %BUILD_DIR%
echo.
set /p CONFIRM=Are you sure you want to continue? (y/N): 

if /i not "%CONFIRM%"=="y" (
    echo Clean operation cancelled.
    echo.
    pause
    exit /b 0
)

echo.
echo Removing build directory...

REM Remove the build directory and all its contents
rmdir /s /q "%BUILD_DIR%"

if %errorlevel% neq 0 (
    echo ERROR: Failed to remove build directory
    echo This might happen if some files are in use
    echo Try closing Visual Studio or any running executables and try again
    echo.
    pause
    exit /b 1
)

echo Build directory removed successfully!
echo.

REM Also clean any additional cache files that might exist in the root
echo Cleaning additional cache files...

if exist "CMakeCache.txt" (
    del "CMakeCache.txt"
    echo Removed CMakeCache.txt
)

if exist "cmake_install.cmake" (
    del "cmake_install.cmake" 
    echo Removed cmake_install.cmake
)

if exist "CMakeFiles" (
    rmdir /s /q "CMakeFiles"
    echo Removed CMakeFiles directory
)

if exist "*.vcxproj" (
    del "*.vcxproj"
    echo Removed Visual Studio project files
)

if exist "*.vcxproj.filters" (
    del "*.vcxproj.filters"
    echo Removed Visual Studio filter files
)

if exist "*.sln" (
    del "*.sln"
    echo Removed Visual Studio solution files
)

echo.
echo =====================================================
echo CLEAN SUMMARY
echo =====================================================
echo All build artifacts have been removed.
echo The project is now in a clean state.
echo.
echo To rebuild the project, run: run.bat
echo.
echo Clean process completed successfully!
pause