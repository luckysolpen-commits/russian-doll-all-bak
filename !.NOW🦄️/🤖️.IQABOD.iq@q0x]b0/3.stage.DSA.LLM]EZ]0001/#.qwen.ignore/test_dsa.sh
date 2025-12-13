#!/bin/bash

# Navigate to the DSA directory
cd '/home/no/Desktop/qwen/dec-6/3.stage.fame+DSA?🐋️🐋️🐋️]a0/3.stage.dsa.clone-a0'

# Initialize the DSA attention model
echo "Initializing DSA attention model..."
./dsa_attention_fixed init dsa_attention_model_test.txt

# Test forward pass with the first word in the vocab
echo "Testing DSA forward pass with first word..."
./dsa_attention_fixed forward dsa_attention_model_test.txt vocab_model.txt 0

echo "DSA implementation test completed!"
