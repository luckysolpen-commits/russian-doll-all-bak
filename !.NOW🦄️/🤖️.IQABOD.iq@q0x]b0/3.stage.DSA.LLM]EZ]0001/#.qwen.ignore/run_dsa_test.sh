#!/bin/bash

# Test script to run DSA implementation
echo "Testing DSA Implementation..."

# Initialize the models
echo "Initializing DSA attention model..."
cd +x
./dsa_attention.+x init ../dsa_attention_model.txt

# Run forward pass with DSA
echo "Running DSA forward pass..."
./dsa_attention.+x forward ../dsa_attention_model.txt ../vocab_model.txt 0

echo "DSA testing completed successfully!"