#!/bin/bash
# main_runner.sh - Run the Keyboard Input Handler

echo "======================================"
echo "TPM Keyboard Input Handler - C Implementation"
echo "Using system() calls and pipes instead of includes/linking"
echo "======================================"

# Compile all modules if needed
echo "Compiling modules..."
./xsh.compile-all.+x.sh
if [ $? -ne 0 ]; then
    echo "Compilation failed"
    exit 1
fi
echo "Compilation completed."

echo ""
echo "Files in pieces/plugins/+x/:"
ls -la pieces/plugins/+x/
echo ""
echo "Piece files:"
find pieces/ -type f
echo ""

echo ""
echo "Starting TPM Keyboard Input Handler..."
echo "Press WASD keys for movement, q to quit"
echo "All keystrokes will be logged to pieces/keyboard/history.txt"
echo ""
./pieces/plugins/+x/keyboard_handler.+x