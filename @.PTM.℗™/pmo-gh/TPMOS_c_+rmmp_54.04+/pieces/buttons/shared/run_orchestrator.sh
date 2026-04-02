#!/bin/bash
# run_orchestrator.sh - Launch CHTPM Orchestrator (Cross-platform)
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

echo "=== CHTPM Orchestrator Launch ==="
echo "Working directory: $(pwd)"
echo

# Setup location_kvp with appropriate paths
mkdir -p pieces/locations

if [ $IS_WINDOWS -eq 1 ]; then
    # Windows: use pwd -W for native Windows paths
    WIN_PATH=$(pwd -W)
    cat > pieces/locations/location_kvp << EOF
project_root=${WIN_PATH}
pieces_dir=${WIN_PATH}\pieces
fuzzpet_app_dir=${WIN_PATH}\pieces\apps\fuzzpet_app
fuzzpet_dir=${WIN_PATH}\pieces\apps\fuzzpet_app\fuzzpet
editor_dir=${WIN_PATH}\pieces\apps\editor
system_dir=${WIN_PATH}\pieces\system
selector_dir=${WIN_PATH}\pieces\world\map_01\selector
os_procs_dir=${WIN_PATH}\pieces\os\procs
clock_daemon_dir=${WIN_PATH}\pieces\system\clock_daemon
manager_dir=${WIN_PATH}\pieces\apps\fuzzpet_app\manager
EOF
    echo "Windows path: ${WIN_PATH}"
else
    # Linux/macOS: use POSIX paths
    POSIX_PATH="$(pwd)"
    cat > pieces/locations/location_kvp << EOF
project_root=${POSIX_PATH}
pieces_dir=${POSIX_PATH}/pieces
fuzzpet_app_dir=${POSIX_PATH}/pieces/apps/fuzzpet_app
fuzzpet_dir=${POSIX_PATH}/pieces/apps/fuzzpet_app/fuzzpet
editor_dir=${POSIX_PATH}/pieces/apps/editor
system_dir=${POSIX_PATH}/pieces/system
selector_dir=${POSIX_PATH}/pieces/world/map_01/selector
os_procs_dir=${POSIX_PATH}/pieces/os/procs
clock_daemon_dir=${POSIX_PATH}/pieces/system/clock_daemon
manager_dir=${POSIX_PATH}/pieces/apps/fuzzpet_app/manager
EOF
    echo "POSIX path: ${POSIX_PATH}"
fi

echo

# Clear state files
> pieces/display/current_frame.txt
> pieces/display/frame_changed.txt
> pieces/keyboard/history.txt
> pieces/apps/player_app/history.txt
> pieces/apps/fuzzpet_app/fuzzpet/history.txt
> pieces/master_ledger/master_ledger.txt

# Initialize display state (for history mode - PARITY FIX)
# Default: history ON (scrolling frames with separators)
echo "on" > pieces/display/state.txt

ORCHESTRATOR="./pieces/chtpm/plugins/+x/orchestrator.+x"

if [ ! -x "$ORCHESTRATOR" ]; then
    echo "ERROR: $ORCHESTRATOR not found or not executable"
    echo "Run compile_all.sh (Linux) or compile_all.ps1 (Windows) first."
    exit 1
fi

echo "Found: $ORCHESTRATOR"
echo "Launching..."
echo "Press Ctrl+C to exit"
echo ========================================

# Execute - bash can run .+x files directly
exec "$ORCHESTRATOR"
