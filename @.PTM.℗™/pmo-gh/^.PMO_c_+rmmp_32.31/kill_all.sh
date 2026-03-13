#!/bin/bash

# kill_all.sh - Surgical TPM Component Cleanup
# Targets specifically binaries in pieces/ and projects/ directories.

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
surgical_kill "renderer"
surgical_kill "gl_renderer"
surgical_kill "clock_daemon"
surgical_kill "response_handler"
surgical_kill "keyboard_input"
surgical_kill "joystick_input"
surgical_kill "fuzzpet_manager"
surgical_kill "playrm_module"
surgical_kill "fuzzpet_v2_module"
surgical_kill "man-ops_module"
surgical_kill "man-pal_module"
surgical_kill "op-ed_module"
surgical_kill "loader_module"
surgical_kill "editor_module"
surgical_kill "move_player"
surgical_kill "move_z"
surgical_kill "interact"
surgical_kill "render_map"
surgical_kill "menu_op"
surgical_kill "project_loader"
surgical_kill "piece_manager"
surgical_kill "prisc"

echo "Cleanup complete."
