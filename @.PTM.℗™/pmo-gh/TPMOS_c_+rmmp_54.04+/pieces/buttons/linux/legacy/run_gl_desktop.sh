#!/bin/bash
# run_gl_desktop.sh - Launch GL-OS Desktop (Cross-platform)
# Works on: Linux, macOS, Windows (MSYS2)

# Detect platform
IS_WINDOWS=0
if [[ "$(uname)" == *"MSYS"* ]] || [[ "$(uname)" == *"MINGW"* ]]; then
    IS_WINDOWS=1
    # Set PATH to include MSYS2 Mingw64 bin directory for DLL dependencies
    export PATH=/c/msys64/mingw64/bin:/c/msys64/usr/bin:$PATH
fi

# Get the directory where this script lives, then go to project root
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR/../../.."

# Change to project root
cd "$PROJECT_ROOT"

echo "=== GL-OS Desktop Debug Launch ==="
echo

# Create required directories
mkdir -p pieces/locations pieces/apps/gl_os/session pieces/debug/frames

# Setup location_kvp with appropriate paths
if [ $IS_WINDOWS -eq 1 ]; then
    # Windows: use pwd -W for native Windows paths
    WIN_PATH=$(pwd -W)
    cat > pieces/locations/location_kvp << EOF
project_root=${WIN_PATH}
pieces_dir=${WIN_PATH}\pieces
system_dir=${WIN_PATH}\pieces\system
gl_os_dir=${WIN_PATH}\pieces\apps\gl_os
EOF
    echo "Windows path: ${WIN_PATH}"
else
    # Linux/macOS: use POSIX paths
    POSIX_PATH="$(pwd)"
    cat > pieces/locations/location_kvp << EOF
project_root=${POSIX_PATH}
pieces_dir=${POSIX_PATH}/pieces
system_dir=${POSIX_PATH}/pieces/system
gl_os_dir=${POSIX_PATH}/pieces/apps/gl_os
EOF
    echo "POSIX path: ${POSIX_PATH}"
fi

GL_DESKTOP="./pieces/apps/gl_os/plugins/+x/gl_desktop.+x"

if [ ! -x "$GL_DESKTOP" ]; then
    echo "ERROR: $GL_DESKTOP not found or not executable"
    echo "Run compile_all.sh (Linux) or compile_all.ps1 (Windows) first."
    exit 1
fi

echo "Found: $(pwd)/$GL_DESKTOP"
echo

echo "Launching gl_desktop.+x..."
echo "Press Ctrl+C to exit"
echo

# Execute - bash can run .+x files directly
exec "$GL_DESKTOP"
