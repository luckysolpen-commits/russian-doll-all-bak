#!/bin/bash

# compile_all.sh - Compile all TPM +X executables

echo "Compiling TPM +X Foundation System..."

# Create the +x directory if it doesn't exist
mkdir -p +x

echo "Compiling piece_manager..."
gcc -o +x/piece_manager.+x plugins/piece_manager.c

echo "Compiling orchestrator..."
gcc -o +x/orchestrator.+x plugins/orchestrator.c -lpthread

echo "Compiling keyboard_input..."
gcc -o +x/keyboard_input.+x plugins/keyboard_input.c

echo "Compiling game_logic..."
gcc -o +x/game_logic.+x plugins/game_logic.c

echo "Compiling renderer..."
gcc -o +x/renderer.+x plugins/renderer.c

echo "Compiling response_handler..."
gcc -o +x/response_handler.+x plugins/response_handler.c

echo "Compiling master_ledger_monitor..."
gcc -o +x/master_ledger_monitor.+x plugins/master_ledger_monitor.c

echo "Compilation complete!"
echo "Executables created in +x/ directory:"
ls -la +x/