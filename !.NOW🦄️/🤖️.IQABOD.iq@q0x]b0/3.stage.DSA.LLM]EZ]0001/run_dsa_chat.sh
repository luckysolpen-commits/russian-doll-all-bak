#!/bin/bash

# Script to run the DSA-enhanced chatbot with user input

# Check if arguments are provided
if [ $# -lt 1 ]; then
    echo "Usage: $0 \"<prompt>\" [length] [temperature]"
    echo "Example: $0 \"apple test\" 3 1.1"
    exit 1
fi

# Set default values
PROMPT="$1"
LENGTH=${2:-10}
TEMPERATURE=${3:-1.0}

echo "Running DSA-enhanced chatbot with prompt: '$PROMPT'"
echo "Length: $LENGTH, Temperature: $TEMPERATURE"
echo ""

# The vocab file is loaded dynamically from the curriculum bank file, 
# so we don't need to check for vocab_model.txt here


# Run the DSA-enhanced chatbot
echo "Generating response with DSA implementation..."
./+x/chatbot_moe_v1.+x meta_curriculum_bank.txt "$PROMPT" $LENGTH $TEMPERATURE

echo ""
echo "DSA-enhanced chatbot execution completed!"
