#!/bin/bash
# run_gl_desktop.sh - Launch GL-OS Desktop on Windows via MSYS2

# Set PATH to include MSYS2 Mingw64 bin directory for DLL dependencies
export PATH=/c/msys64/mingw64/bin:/c/msys64/usr/bin:$PATH

cd "$(dirname "$0")"

echo "=== GL-OS Desktop Debug Launch ==="
echo

# Create required directories
mkdir -p pieces/locations pieces/apps/gl_os/session pieces/debug/frames

# Get Windows path using pwd -W (native Windows path)
# pwd -W returns C:/Users/... format which Windows C code accepts
WIN_PATH=$(pwd -W)

# Setup location_kvp with Windows-style paths (C:\...)
# The C code on Windows needs Windows paths, not /c/...
echo "project_root=${WIN_PATH}" > pieces/locations/location_kvp
echo "pieces_dir=${WIN_PATH}\pieces" >> pieces/locations/location_kvp
echo "system_dir=${WIN_PATH}\pieces\system" >> pieces/locations/location_kvp
echo "gl_os_dir=${WIN_PATH}\pieces\apps\gl_os" >> pieces/locations/location_kvp

GL_DESKTOP="./pieces/apps/gl_os/plugins/+x/gl_desktop.+x"

if [ ! -x "$GL_DESKTOP" ]; then
    echo "ERROR: $GL_DESKTOP not found or not executable"
    exit 1
fi

echo "Found: $(pwd)/$GL_DESKTOP"
echo "Windows path: ${WIN_PATH}"
echo

echo "Launching gl_desktop.+x..."
echo "Press Ctrl+C to exit"
echo

# Execute - bash can run .+x files directly
exec "$GL_DESKTOP"
