#!/bin/bash

# Check if a filename was provided
if [ -z "$1" ]; then
    echo "Usage: $0 <filename>"
    exit 1
fi

# Prepend ../ to every line using sed
# ^ matches the start of the line; we use # as a delimiter to avoid "slash soup"
sed -i 's#^#../#' "$1"

echo "Done! Prepended '../' to every line in $1."

