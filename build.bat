@echo off
setlocal
REM Always work from the folder this script lives in (also when double-clicked)
cd /d "%~dp0"

echo ============================================
echo  Building Jello
echo  SFML 3 is downloaded and built automatically if it is not installed.
echo  The first build takes several minutes and needs git and internet access.
echo ============================================
echo.

echo [1/2] Configuring with CMake...
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 (
    echo.
    echo CMake configuration failed! Check the messages above.
    pause
    exit /b 1
)

echo.
echo [2/2] Compiling...
cmake --build build --config Release -j
if errorlevel 1 (
    echo.
    echo Build failed! Check the messages above.
    pause
    exit /b 1
)

echo.
echo Build completed successfully!
echo Executable: release\Jello.exe
endlocal
