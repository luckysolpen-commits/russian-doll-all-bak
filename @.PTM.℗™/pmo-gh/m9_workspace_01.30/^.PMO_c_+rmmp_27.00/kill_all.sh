#!/bin/bash

# kill_all.sh - Surgical TPM Component Cleanup
# Targets specifically binaries in pieces/ directories.

# Function for surgical killing
surgical_kill() {
    local name="$1"
    # Match the specific path pieces/...+x/name.+x to avoid killing browsers
    local pattern="pieces/.*/\+x/${name}\.\+x"
    if pgrep -f "$pattern" > /dev/null; then
        echo "Killing $name..."
        pkill -9 -f "$pattern"
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
surgical_kill "loader_module"
surgical_kill "editor_module"
surgical_kill "move_player"
surgical_kill "move_z"
surgical_kill "interact"
surgical_kill "render_map"
surgical_kill "menu_op"
surgical_kill "project_loader"
surgical_kill "piece_manager"

echo "Cleanup complete."
