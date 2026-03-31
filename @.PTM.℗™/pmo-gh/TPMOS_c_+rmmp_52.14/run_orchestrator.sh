#!/bin/bash
# run_orchestrator.sh - Launch CHTPM Orchestrator on Windows via MSYS2

# Set PATH to include MSYS2 Mingw64 bin directory for DLL dependencies
export PATH=/c/msys64/mingw64/bin:/c/msys64/usr/bin:$PATH

cd "$(dirname "$0")"

echo "=== CHTPM Orchestrator Launch ==="
echo "Working directory: $(pwd)"
echo

# Get Windows path using pwd -W (native Windows path)
# pwd -W returns C:/Users/... format which Windows C code accepts
WIN_PATH=$(pwd -W)

# Setup location_kvp with Windows-style paths
mkdir -p pieces/locations
echo "project_root=${WIN_PATH}" > pieces/locations/location_kvp
echo "pieces_dir=${WIN_PATH}\pieces" >> pieces/locations/location_kvp
echo "fuzzpet_app_dir=${WIN_PATH}\pieces\apps\fuzzpet_app" >> pieces/locations/location_kvp
echo "fuzzpet_dir=${WIN_PATH}\pieces\apps\fuzzpet_app\fuzzpet" >> pieces/locations/location_kvp
echo "editor_dir=${WIN_PATH}\pieces\apps\editor" >> pieces/locations/location_kvp
echo "system_dir=${WIN_PATH}\pieces\system" >> pieces/locations/location_kvp
echo "selector_dir=${WIN_PATH}\pieces\world\map_01\selector" >> pieces/locations/location_kvp
echo "os_procs_dir=${WIN_PATH}\pieces\os\procs" >> pieces/locations/location_kvp
echo "clock_daemon_dir=${WIN_PATH}\pieces\system\clock_daemon" >> pieces/locations/location_kvp
echo "manager_dir=${WIN_PATH}\pieces\apps\fuzzpet_app\manager" >> pieces/locations/location_kvp

echo "Windows path: ${WIN_PATH}"
echo

# Clear state files
> pieces/display/current_frame.txt
> pieces/display/frame_changed.txt
> pieces/keyboard/history.txt
> pieces/master_ledger/master_ledger.txt

ORCHESTRATOR="./pieces/chtpm/plugins/+x/orchestrator.+x"

if [ ! -x "$ORCHESTRATOR" ]; then
    echo "ERROR: $ORCHESTRATOR not found or not executable"
    echo "Run compile_all.ps1 first."
    exit 1
fi

echo "Found: $ORCHESTRATOR"
echo "Launching..."
echo "Press Ctrl+C to exit"
echo ========================================

# Execute - bash can run .+x files directly
exec "$ORCHESTRATOR"
