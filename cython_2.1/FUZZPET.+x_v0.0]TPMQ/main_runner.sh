#!/bin/bash
# main_runner.sh - Run the FuzzPet TPM Orchestration System

echo "======================================"
echo "FuzzPet TPM Orchestration System - Independent Modules"
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
echo "Starting FuzzPet TPM Orchestration System..."
echo "Commands: feed, play, sleep, status, evolve, quit"
echo "Shortcuts: 1=feed, 2=play, 3=sleep, 4=status, 5=evolve, 6=quit"
echo "Ctrl+C to quit"
echo ""
./pieces/plugins/+x/orchestrator.+x