#!/bin/bash
# Compile TPM C Keyboard System

echo "Compiling TPM C Keyboard System..."

# Create bin directory if not exists
mkdir -p bin

# Compile the main executables
gcc -std=c99 -Wall -Wextra -o bin/tpm_keyboard main.c
gcc -std=c99 -Wall -Wextra -o bin/history_monitor history_monitor.c
gcc -std=c99 -Wall -Wextra -o bin/keyboard_simulator keyboard_simulator.c

echo "Compilation complete!"
echo "Run with: ./bin/tpm_keyboard (requires terminal)"
echo "Or run: ./bin/history_monitor (monitor only)"
echo "Or run: ./bin/keyboard_simulator (input simulator)"