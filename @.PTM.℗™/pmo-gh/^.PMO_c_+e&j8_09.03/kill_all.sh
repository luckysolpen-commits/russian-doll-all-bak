#!/bin/bash
# kill_all.sh - Terminate all CHTPM project processes

echo "Cleaning up orphaned Piecemark components..."
# Kill all binaries with .+x extension
pkill -f "\.\+x"
# Kill by name just in case
pkill -f orchestrator
pkill -f chtpm_parser
pkill -f fuzzpet_manager
pkill -f renderer
pkill -f gl_renderer
pkill -f keyboard_input
pkill -f joystick_input
pkill -f response_handler
pkill -f proc_manager

# Clear PIDs directory
rm -f pieces/os/procs/*.pid

echo "Cleanup Complete."
