#!/bin/bash

# Create debug directory if it doesn't exist
mkdir -p "#.debug"

# List of debug files that need to be moved
DEBUG_FILES=(
    "debug.log"
    "frames.txt"
    "dom_refresh_debug.log"
    "render_debug.log"
    "init_debug.txt"
    "display_debug.txt"
    "load_debug.txt"
    "module_debug.log"
    "map_from_module.txt"
)

# Move each debug file to the debug directory if it exists
for file in "${DEBUG_FILES[@]}"; do
    if [ -f "$file" ]; then
        echo "Moving $file to #.debug/ directory..."
        mv "$file" "#.debug/"
    fi
done

echo "Debug files have been moved to #.debug/ directory."