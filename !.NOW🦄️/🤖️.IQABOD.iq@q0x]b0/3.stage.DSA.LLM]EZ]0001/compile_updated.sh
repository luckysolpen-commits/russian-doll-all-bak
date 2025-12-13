#!/bin/bash

# Compile all the components for the DSA LLM with updated sequence training

echo "Compiling sequence operations..."
gcc -std=c99 -O3 -o sequence_ops sequence_ops.c -lm

echo "Compiling updated rope.c..."
gcc -std=c99 -O3 -o rope rope.c -lm

echo "Compiling trainer.c with sequence capabilities..."
gcc -std=c99 -O3 -o trainer trainer.c -lm

echo "Compiling other components..."
gcc -std=c99 -O3 -o forward_prop forward_prop.c -lm
gcc -std=c99 -O3 -o backward_prop backward_prop.c -lm
gcc -std=c99 -O3 -o optimizer optimizer.c -lm

# Create executable directory if it doesn't exist
mkdir -p +x

# Move executables to +x directory
mv sequence_ops rope trainer forward_prop backward_prop optimizer +x/

echo "Compilation complete! All executables are in the +x/ directory."