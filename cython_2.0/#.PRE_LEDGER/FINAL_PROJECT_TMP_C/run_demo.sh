#!/bin/bash

# TPM C Roguelike Game - Interactive Demo Script

echo "========================================="
echo "TPM C Roguelike Game"
echo "True Piece Method Architecture Demo"
echo "========================================="
echo ""

echo "Project Structure:"
echo "- src/: Contains C source modules"
echo "- peices/: True Piece Method data files"
echo "- bin/: Compiled executables"  
echo ""

echo "Available Executables:"
echo "- bin/roguelike: The main game executable"
echo ""

echo "Piece Files:"
echo "- peices/world/map.txt: Game map data"
echo "- peices/world/entities.txt: Entity placements"
echo "- peices/player/player.txt: Player state"
echo "- peices/menu/menu.txt: Menu options" 
echo ""

echo "To run the game:"
echo "$ cd /home/no/Desktop/qwen/#.fbx.fix.fund+4/0.compliant-up/fix-kb/remove-ascimoji-keyboard/FINAL_PROJECT_C"
echo "$ ./bin/roguelike"
echo ""
echo "Controls:"
echo "- WASD or Arrow Keys: Move player"
echo "- 0-9: Menu options"
echo "- Ctrl+C: Exit game"
echo ""

echo "Note: The game should display an ASCII map with @"
echo "player symbol and other entities like Z (zombies)"
echo "and & (sheep). The display updates when you move."
echo ""

echo "Current status: Game compiles successfully and shows map"
echo "when run in a terminal environment."
echo "========================================="

# Ask user if they want to try the game
read -p "Would you like to try running the game now? (y/n): " choice
if [[ $choice =~ ^[Yy]$ ]]; then
    echo "Changing to game directory..."
    cd /home/no/Desktop/qwen/#.fbx.fix.fund+4/0.compliant-up/fix-kb/remove-ascimoji-keyboard/FINAL_PROJECT_C
    echo "Running the game (Press Ctrl+C to quit)..."
    ./bin/roguelike
fi