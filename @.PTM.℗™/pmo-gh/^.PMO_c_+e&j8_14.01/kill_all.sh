#!/bin/bash

# kill_all.sh - Surgical TPM Component Cleanup
# Targets specifically binaries in pieces/ directories.

# Function for surgical killing
surgical_kill() {
    local pattern="pieces/.*$1"
    # pgrep -f to find, pkill -f to kill
    if pgrep -f "$pattern" > /dev/null; then
        echo "Killing $1..."
        pkill -f "$pattern"
    fi
}

surgical_kill "orchestrator\.+x"
surgical_kill "chtpm_parser\.+x"
surgical_kill "renderer\.+x"
surgical_kill "gl_renderer\.+x"
surgical_kill "clock_daemon\.+x"
surgical_kill "response_handler\.+x"
surgical_kill "keyboard_input\.+x"
surgical_kill "joystick_input\.+x"
surgical_kill "fuzzpet_manager\.+x"
surgical_kill "move_entity\.+x"
surgical_kill "map_render\.+x"
surgical_kill "piece_manager\.+x"

# Also clean up any shell wrappers (but not this one)
# pkill -f "run_chtpm.sh"
