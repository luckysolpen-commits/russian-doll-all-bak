#!/bin/bash

# compile_all.sh - Compile all TPM +X executables
# TPM-Compliant: Binaries go to pieces/<piece>/plugins/+x/

echo "Compiling TPM +X Foundation System..."

# Create the +x directory for orchestrator
mkdir -p +x

echo "Compiling keyboard_input..."
gcc -o pieces/keyboard/plugins/+x/keyboard_input.+x pieces/keyboard/plugins/keyboard_input.c

echo "Compiling renderer..."
gcc -o pieces/display/plugins/+x/renderer.+x pieces/display/plugins/renderer.c

echo "Compiling gl_renderer..."
gcc -o pieces/display/plugins/+x/gl_renderer.+x pieces/display/plugins/gl_renderer.c -lGL -lglut `pkg-config --cflags --libs freetype2`

echo "Compiling increment_time..."
gcc -o pieces/clock/plugins/+x/increment_time.+x pieces/clock/plugins/increment_time.c

echo "Compiling increment_turn..."
gcc -o pieces/clock/plugins/+x/increment_turn.+x pieces/clock/plugins/increment_turn.c

echo "Compiling end_turn..."
gcc -o pieces/clock/plugins/+x/end_turn.+x pieces/clock/plugins/end_turn.c

echo "Compiling game_logic (with optional PDLO routing)..."
# Default: compile with fallback mode (works without PDLO parser)
gcc -o pieces/world/plugins/+x/game_logic.+x pieces/world/plugins/game_logic.c
# Optional: compile with full PDLO routing (uncomment to use)
# gcc -DUSE_PDLO_ROUTING -o pieces/world/plugins/+x/game_logic.+x pieces/world/plugins/game_logic.c

echo "Compiling piece_manager..."
gcc -o pieces/master_ledger/plugins/+x/piece_manager.+x pieces/master_ledger/plugins/piece_manager.c

echo "Compiling master_ledger_monitor..."
gcc -o pieces/master_ledger/plugins/+x/master_ledger_monitor.+x pieces/master_ledger/plugins/master_ledger_monitor.c

echo "Compiling response_handler..."
gcc -o pieces/master_ledger/plugins/+x/response_handler.+x pieces/master_ledger/plugins/response_handler.c

echo "Compiling orchestrator..."
gcc -o pieces/system/plugins/+x/orchestrator.+x pieces/system/plugins/orchestrator.c -lpthread

echo ""
echo "Compilation complete!"
echo "Executables created:"
find pieces -name "*.+x" -type f
ls -la +x/
