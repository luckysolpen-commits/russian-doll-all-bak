#!/bin/bash

# Define the output directory
output_dir="+x"
button_dir="./"

# Create the +x/ directory if it doesn't exist
if [ ! -d "$output_dir" ]; then
    mkdir "$output_dir"
    if [ $? -ne 0 ]; then
        echo "Error: Could not create directory $output_dir"
        exit 1
    fi
fi

# Compile specific modules with appropriate flags
echo "Compiling vocab_module.c..."
gcc vocab_module.c -o "$output_dir/vocab_module.+x" -lm -O3
if [ $? -eq 0 ]; then
    echo "Successfully compiled vocab_module.c into $output_dir/vocab_module.+x"
else
    echo "Error compiling vocab_module.c"
fi

echo "Compiling rope_module.c..."
gcc rope_module.c -o "$output_dir/rope_module.+x" -lm -O3
if [ $? -eq 0 ]; then
    echo "Successfully compiled rope_module.c into $output_dir/rope_module.+x"
else
    echo "Error compiling rope_module.c"
fi

echo "Compiling feedforward_module.c..."
gcc feedforward_module.c -o "$output_dir/feedforward_module.+x" -lm -O3
if [ $? -eq 0 ]; then
    echo "Successfully compiled feedforward_module.c into $output_dir/feedforward_module.+x"
else
    echo "Error compiling feedforward_module.c"
fi

echo "Compiling attention_module.c..."
gcc attention_module.c -o "$output_dir/attention_module.+x" -lm -O3
if [ $? -eq 0 ]; then
    echo "Successfully compiled attention_module.c into $output_dir/attention_module.+x"
else
    echo "Error compiling attention_module.c"
fi

echo "Compiling backprop_module.c..."
gcc backprop_module.c -o "$output_dir/backprop_module.+x" -lm -O3
if [ $? -eq 0 ]; then
    echo "Successfully compiled backprop_module.c into $output_dir/backprop_module.+x"
else
    echo "Error compiling backprop_module.c"
fi

echo "Compiling feedback_tx_module.c..."
gcc feedback_tx_module.c -o "$output_dir/feedback_tx_module.+x" -lm -O3
if [ $? -eq 0 ]; then
    echo "Successfully compiled feedback_tx_module.c into $output_dir/feedback_tx_module.+x"
else
    echo "Error compiling feedback_tx_module.c"
fi

echo "Compiling generation_module.c..."
gcc generation_module.c -o "$output_dir/generation_module.+x" -lm -O3
if [ $? -eq 0 ]; then
    echo "Successfully compiled generation_module.c into $output_dir/generation_module.+x"
else
    echo "Error compiling generation_module.c"
fi

echo "Compiling main_orchestrator.c..."
gcc main_orchestrator.c -o "$output_dir/main_orchestrator.+x" -lm -O3
if [ $? -eq 0 ]; then
    echo "Successfully compiled main_orchestrator.c into $output_dir/main_orchestrator.+x"
else
    echo "Error compiling main_orchestrator.c"
fi

# Also compile the original model if it still exists
if [ -f "mirror_model_v6_with_backprop.c" ]; then
    echo "Compiling mirror_model_v6_with_backprop.c..."
    gcc mirror_model_v6_with_backprop.c -o "$output_dir/mirror_model_v6_with_backprop.+x" -lm -O3
    if [ $? -eq 0 ]; then
        echo "Successfully compiled mirror_model_v6_with_backprop.c into $output_dir/mirror_model_v6_with_backprop.+x"
    else
        echo "Error compiling mirror_model_v6_with_backprop.c"
    fi
fi

# Also compile run]karp=999.loc.c if it still exists
if [ -f "run]karp=999.loc.c" ]; then
    echo "Compiling run]karp=999.loc.c..."
    gcc run]karp=999.loc.c -o "$output_dir/run]karp=999.loc.+x" -lm -O3
    if [ $? -eq 0 ]; then
        echo "Successfully compiled run]karp=999.loc.c into $output_dir/run]karp=999.loc.+x"
    else
        echo "Error compiling run]karp=999.loc.c"
    fi
fi

echo "Compilation complete."


