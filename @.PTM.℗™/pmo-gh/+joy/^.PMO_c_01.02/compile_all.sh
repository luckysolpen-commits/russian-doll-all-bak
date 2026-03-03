#!/bin/bash

# compile_all.sh - Compile all CHTPM v0.01 executables
# TPM-Compliant: Binaries go to their respective piece directories.
# Updated: Macro folder structure (apps/fuzzpet_app/)

echo "=== Compiling CHTPM v0.01 (File-Based Communication) ==="
echo "Using macro folder structure: pieces/apps/fuzzpet_app/"
echo ""

# --- FuzzPet Module (in macro folder) ---
echo "Compiling fuzzpet_app/fuzzpet..."
mkdir -p pieces/apps/fuzzpet_app/fuzzpet/plugins/+x
gcc -o pieces/apps/fuzzpet_app/fuzzpet/plugins/+x/fuzzpet.+x pieces/apps/fuzzpet_app/fuzzpet/plugins/fuzzpet.c

echo "Compiling fuzzpet_app/fuzzpet_reset..."
mkdir -p pieces/apps/fuzzpet_app/fuzzpet/plugins/+x
gcc -o pieces/apps/fuzzpet_app/fuzzpet/plugins/+x/fuzzpet_reset.+x pieces/apps/fuzzpet_app/fuzzpet/plugins/fuzzpet_reset.c

# --- Keyboard Piece (system level) ---
echo "Compiling keyboard/keyboard_input..."
mkdir -p pieces/keyboard/plugins/+x
gcc -o pieces/keyboard/plugins/+x/keyboard_input.+x pieces/keyboard/plugins/keyboard_input.c

# --- Joystick Piece (system level) ---
echo "Compiling joystick/joystick_input..."
mkdir -p pieces/joystick/plugins/+x
gcc -o pieces/joystick/plugins/+x/joystick_input.+x pieces/joystick/plugins/joystick_input.c

# --- CHTPM Piece (system level) ---
echo "Compiling chtpm/chtpm_parser..."
mkdir -p pieces/chtpm/plugins/+x
gcc -o pieces/chtpm/plugins/+x/chtpm_parser.+x pieces/chtpm/plugins/chtpm_parser.c 2>&1 | grep -v "warning:"

echo "Compiling chtpm/orchestrator..."
mkdir -p pieces/chtpm/plugins/+x
gcc -o pieces/chtpm/plugins/+x/orchestrator.+x pieces/chtpm/plugins/orchestrator.c -pthread

# --- Display Piece (system level) ---
echo "Compiling display/renderer..."
mkdir -p pieces/display/plugins/+x
gcc -o pieces/display/plugins/+x/renderer.+x pieces/display/renderer.c

echo "Compiling display/gl_renderer..."
mkdir -p pieces/display/plugins/+x
gcc -o pieces/display/plugins/+x/gl_renderer.+x pieces/display/gl_renderer.c -lGL -lglut 2>/dev/null || echo "  (GL renderer skipped)"

# --- Master Ledger Pieces (system level) ---
echo "Compiling master_ledger/piece_manager..."
mkdir -p pieces/master_ledger/plugins/+x
gcc -o pieces/master_ledger/plugins/+x/piece_manager.+x pieces/master_ledger/plugins/piece_manager.c

echo "Compiling master_ledger/response_handler..."
mkdir -p pieces/master_ledger/plugins/+x
gcc -o pieces/master_ledger/plugins/+x/response_handler.+x pieces/master_ledger/plugins/response_handler.c

# --- Clock Piece (in macro folder) ---
echo "Compiling fuzzpet_app/clock/increment_time..."
mkdir -p pieces/apps/fuzzpet_app/clock/plugins/+x
gcc -o pieces/apps/fuzzpet_app/clock/plugins/+x/increment_time.+x pieces/apps/fuzzpet_app/clock/plugins/increment_time.c

echo "Compiling fuzzpet_app/clock/increment_turn..."
mkdir -p pieces/apps/fuzzpet_app/clock/plugins/+x
gcc -o pieces/apps/fuzzpet_app/clock/plugins/+x/increment_turn.+x pieces/apps/fuzzpet_app/clock/plugins/increment_turn.c

echo "Compiling fuzzpet_app/clock/end_turn..."
mkdir -p pieces/apps/fuzzpet_app/clock/plugins/+x
gcc -o pieces/apps/fuzzpet_app/clock/plugins/+x/end_turn.+x pieces/apps/fuzzpet_app/clock/plugins/end_turn.c

# --- World Piece (in macro folder) ---
echo "Compiling fuzzpet_app/world (no plugins - data only)"

# --- Locations Piece (system level) ---
echo "Compiling path_utils..."
mkdir -p pieces/locations/+x
gcc -o pieces/locations/+x/path_utils.+x pieces/locations/path_utils.c

echo ""
echo "=== Compilation Complete ==="
echo "Executables created:"
find pieces -name "*.+x" -type f 2>/dev/null
