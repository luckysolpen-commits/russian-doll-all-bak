#!/bin/bash
# build_bot.sh for Exo-Testing-Bot v4
# Minimal script to compile all C files into a single executable.
# Assumes C files and headers are in the same directory or its subdirs.

# Define source files and output executable
BOT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )/.." &> /dev/null && pwd )"
SOURCES="$BOT_DIR/main.c $BOT_DIR/fsm.c $BOT_DIR/tpmos_interaction.c $BOT_DIR/child_process_sim.c"
OBJECTS=""

# Include paths: current bot base directory for local headers (if any are added later)
INCLUDE_DIRS="-I'$BOT_DIR'" 

OUTPUT_EXECUTABLE="$BOT_DIR/exo-testing-bot-v4"

# --- Compilation ---
echo "Starting compilation..."

# Compile each source file into an object file in a dedicated obj directory
OBJ_DIR="$BOT_DIR/obj"
mkdir -p "$OBJ_DIR" # Ensure obj directory exists

for src_file in $SOURCES; do
    # Extract filename from path
    filename=$(basename -- "$src_file")
    # Create object file path in obj directory
    obj_file="$OBJ_DIR/${filename%.c}.o"
    OBJECTS="$OBJECTS $obj_file"
    
    echo "Compiling $src_file..."
    # Use -c to compile only, -o for output object file, -g for debug info
    # Adding -pthread as child process simulation might use threads, though not strictly required for current simulation.
    if gcc -Wall -Wextra -g -c "$src_file" -o "$obj_file" $INCLUDE_DIRS -pthread; then
        echo "$src_file compiled successfully."
    else
        echo "Error compiling $src_file. Aborting."
        exit 1
    fi
done

# --- Linking ---
echo "Linking object files..."
if gcc $OBJECTS -o "$OUTPUT_EXECUTABLE"; then
    echo "Linking successful. Executable created at $OUTPUT_EXECUTABLE."
else
    echo "Error during linking. Aborting."
    exit 1
fi

# --- Make executable runnable ---
chmod +x "$OUTPUT_EXECUTABLE"

echo "Build process completed successfully."
