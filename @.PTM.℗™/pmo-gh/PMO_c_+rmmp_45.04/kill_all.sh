#!/bin/bash

# kill_all.sh - Surgical TPM Component Cleanup
# Targets specifically binaries in pieces/ and projects/ directories.

# Optional: Run diagnostic first if check script exists
if [ -f "#.tools/check_pmo_procs.sh" ]; then
    echo "=== Running PMO Process Diagnostic ==="
    bash "#.tools/check_pmo_procs.sh"
    echo ""
fi

# Function for surgical killing
surgical_kill() {
    local name="$1"
    # Match specific paths to avoid killing browsers/system tools
    local pattern_pieces="pieces/.*/\+x/${name}\.\+x"
    local pattern_projects="projects/.*/\+x/${name}\.\+x"
    local pattern_system="pieces/system/.*/${name}"
    
    if pgrep -f "$pattern_pieces" > /dev/null; then
        echo "Killing $name (pieces)..."
        pkill -9 -f "$pattern_pieces"
    fi
    if pgrep -f "$pattern_projects" > /dev/null; then
        echo "Killing $name (projects)..."
        pkill -9 -f "$pattern_projects"
    fi
    if pgrep -f "$pattern_system" > /dev/null; then
        echo "Killing $name (system)..."
        pkill -9 -f "$pattern_system"
    fi
}

echo "Cleaning environment..."
surgical_kill "orchestrator"
surgical_kill "chtpm_parser"
surgical_kill "chtpm_player"
surgical_kill "renderer"
surgical_kill "gl_renderer"
surgical_kill "clock_daemon"
surgical_kill "response_handler"
surgical_kill "keyboard_input"
surgical_kill "joystick_input"
surgical_kill "input_capture"
surgical_kill "fuzzpet_manager"
surgical_kill "fuzz_legacy_manager"
surgical_kill "playrm_module"
surgical_kill "fuzzpet_v2_module"
surgical_kill "man-ops_module"
surgical_kill "man-pal_module"
surgical_kill "man-add_module"
surgical_kill "op-ed_module"
surgical_kill "loader_module"
surgical_kill "editor_module"
surgical_kill "db_editor_module"
surgical_kill "pal_editor_module"
surgical_kill "move_player"
surgical_kill "move_z"
surgical_kill "interact"
surgical_kill "render_map"
surgical_kill "menu_op"
surgical_kill "project_loader"
surgical_kill "piece_manager"
surgical_kill "proc_manager"
surgical_kill "pdl_reader"
surgical_kill "ai_manager"
surgical_kill "player_manager"
surgical_kill "player_render"
surgical_kill "prisc"
surgical_kill "gl_os"
surgical_kill "gl_os_manager"
surgical_kill "gl_os_renderer"

# Kill Watchdog
pkill -9 -f "pal_watchdog.sh" 2>/dev/null

# Nuclear Option: Kill ANY residual .+x process in the workspace
echo "Checking for residual .+x processes..."
pkill -9 -f "pieces/.*/\+x/.*\.+x" 2>/dev/null
pkill -9 -f "projects/.*/\+x/.*\.+x" 2>/dev/null

echo "Cleanup complete."
