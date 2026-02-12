#!/bin/bash
# main_runner.sh - Run the TPM Orchestration System

echo "======================================"
echo "TPM Orchestration System - Independent Modules"
echo "Using system() calls and pipes instead of includes/linking"
echo "======================================"

# Compile orchestrator if needed
if [ ! -f "orchestrator.exe" ]; then
    echo "Compiling orchestrator..."
    gcc -o orchestrator.exe orchestrator.c
    if [ $? -ne 0 ]; then
        echo "Failed to compile orchestrator.c"
        exit 1
    fi
    echo "Orchestrator compiled successfully"
else
    echo "Using existing orchestrator.exe"
fi

echo ""
echo "Files in +x/:"
ls -la +x/
echo ""
echo "Piece files:"
find peices/ -type f
echo ""

echo ""
echo "Starting TPM Orchestration System..."
echo "Controls: WASD or arrow keys to move, 0-9 for menu, Ctrl+C to quit"
echo ""
./orchestrator.exe