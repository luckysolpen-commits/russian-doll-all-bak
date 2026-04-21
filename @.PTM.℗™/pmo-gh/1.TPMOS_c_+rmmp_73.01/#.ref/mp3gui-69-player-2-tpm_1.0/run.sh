#!/bin/bash

# Ensure we are in the script directory
cd "$(dirname "$0")"

# Copy config if not present
if [ ! -f "config.txt" ]; then
    echo "Local config.txt not found, copying from JukeBox..."
    cp "../🎛️.juke-box=money-sink-NOW!/config.txt" .
fi

# 1. Find MP3s and build playlist (will read from config.txt automatically)
echo "Finding music using config.txt..."
./yingyang-find

if [ ! -f "playlist.txt" ]; then
    echo "No music found. Exiting."
    exit 1
fi

# 2. Clear old status/command files
rm -f player_command.txt player_status.txt player_viz.dat

# 3. Start CLI backend in background
echo "Starting audio engine..."
./yingyang-cli &
CLI_PID=$!

# 4. Start GUI frontend
echo "Starting GUI..."
./yingyang-gui

# Cleanup on exit
echo "Shutting down..."
kill $CLI_PID
rm -f player_command.txt player_status.txt player_viz.dat
