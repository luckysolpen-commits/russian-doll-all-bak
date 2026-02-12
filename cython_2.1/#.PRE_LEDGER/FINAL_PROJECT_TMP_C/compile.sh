#!/bin/bash
# Compile TPM C Roguelike Game

echo "Compiling TPM C Roguelike Game..."

# Create bin directory if not exists
mkdir -p bin

# Compile the main executable with GNU source definitions for proper POSIX functions
gcc -std=c99 -D_GNU_SOURCE -Wall -Wextra -o bin/roguelike src/main.c

echo "Compilation complete!"
echo "Run with: ./bin/roguelike (requires terminal)"