#!/bin/bash

# Simple test to run DSA attention and verify it works

echo "Testing DSA attention implementation..."

# Navigate to the DSA directory
cd '/home/no/Desktop/qwen/dec-6/3.stage.fame+DSA?🐋️🐋️🐋️]a0/3.stage.dsa.clone-a0'

# Run a simple test
echo "Initializing DSA attention model..."
./+x/dsa_attention.+x init test_dsa_model.txt

echo "Running forward pass with DSA attention..."
./+x/dsa_attention.+x forward test_dsa_model.txt vocab_model.txt 0

echo "DSA attention test completed successfully!"