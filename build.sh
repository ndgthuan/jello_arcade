#!/bin/bash
# Always work from the folder this script lives in
cd "$(dirname "$0")" || exit 1

echo "============================================"
echo " Building Jello"
echo " SFML 3 is downloaded and built automatically if it is not installed."
echo " The first build takes several minutes and needs git and internet access."
echo "============================================"
echo

echo "[1/2] Configuring with CMake..."
if ! cmake -S . -B build -DCMAKE_BUILD_TYPE=Release; then
    echo "CMake configuration failed! Check the messages above."
    exit 1
fi

echo
echo "[2/2] Compiling..."
if ! cmake --build build --config Release -j; then
    echo "Build failed! Check the messages above."
    exit 1
fi

echo
echo "Build completed successfully!"
echo "Executable: release/Jello"
