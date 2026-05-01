#!/bin/bash

# This script translates the entire Chinese bible file (bible]ch.txt)
# using the 'translate-shell' tool (trans) and saves the output to a
# new file (bible.en_translation.txt), preserving the original verse metadata.
# This version supports resuming from where it stopped.

INPUT_FILE="bible]ch.txt"
OUTPUT_FILE="bible.en_translation.txt"

# Check if the input file exists
if [ ! -f "$INPUT_FILE" ]; then
    echo "Error: Input file '$INPUT_FILE' not found."
    exit 1
fi

# Check if 'trans' command is available
if ! command -v trans &> /dev/null; then
    echo "Error: 'translate-shell' (trans) command not found. Please make sure it is installed and in your PATH."
    exit 1
fi

# Count how many lines are already translated in the output file
if [ -f "$OUTPUT_FILE" ]; then
    LINES_DONE=$(wc -l < "$OUTPUT_FILE")
    START_LINE=$((LINES_DONE + 1))
    echo "Found existing translation with $LINES_DONE lines. Resuming from line $START_LINE..."
else
    START_LINE=1
    echo "Starting fresh translation..."
    > "$OUTPUT_FILE"  # Create/clear the output file
fi

# Count total lines in the input file
TOTAL_LINES=$(wc -l < "$INPUT_FILE")
echo "Total lines in input file: $TOTAL_LINES"

echo "Starting translation of '$INPUT_FILE'..."
echo "This will take a significant amount of time, please be patient."

# Use a counter to track current line
current_line=1
while IFS= read -r line; do
    # Skip lines that are already processed
    if [ "$current_line" -lt "$START_LINE" ]; then
        ((current_line++))
        continue
    fi

    if [[ -z "$line" ]]; then
        # If the line is empty, just add a newline to the output
        echo "" >> "$OUTPUT_FILE"
    else
        # Separate the metadata prefix from the Chinese text
        prefix=$(echo "$line" | cut -d'|' -f1-4)"||"
        chinese_text=$(echo "$line" | cut -d'|' -f5-)

        # Translate the Chinese text to English
        # The -brief flag is used to get just the translation text
        translated_text=$(trans -brief :en "$chinese_text")

        # Combine the prefix and the translated text and append to the output file
        echo "${prefix}${translated_text}" >> "$OUTPUT_FILE"
    fi

    # Show progress
    if [ $((current_line % 10)) -eq 0 ]; then
        percentage=$(awk "BEGIN {printf \"%.2f\", ${current_line}*100/${TOTAL_LINES}}")
        echo "Progress: $current_line/$TOTAL_LINES lines ($percentage%)"
    fi

    # Print a dot for each line processed to show progress
    printf "."

    ((current_line++))
done < "$INPUT_FILE"

echo "" # Newline after the dots
echo "Translation complete. Output saved to '$OUTPUT_FILE'."