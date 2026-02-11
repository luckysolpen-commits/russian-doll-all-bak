# ASCII Roguelike Game - FINAL PROJECT

This is the final ASCII roguelike game built using TPM (True Piece Method) architecture.

## Features

- ASCII-based world view with player character (@)
- World loaded from text-based map files
- NPC entities that move around the map
- Integrated menu system accessible with number keys
- WASD or arrow key movement
- Persistent game state stored in piece files
- Event-driven architecture using TPM principles

## Controls

- **W/A/S/D** or **Arrow Keys** - Move player character
- **0-9** - Select menu options (0=end_turn, 1=hunger, 2=state, 3=act, 4=inventory)
- **Q** - Quit game

## Architecture

The game follows a plug-in based architecture similar to the original system:

- `core.world`: Handles world map loading and management
- `core.player`: Manages player character and movement
- `core.control`: Processes keyboard input (movement and menu)
- `ui.display`: Renders the ASCII map and menu interface
- `core.event_manager`: Handles scheduled events and responses
- Other core plugins: turn, health, action, inventory, etc.

## Storage

All game state is stored in human-readable text files under the `games/default/pieces/` directory following TPM architecture principles.

## Files

- `main.py` - Main game engine and entry point
- `plugin_interface.py` - Defines the plugin interface
- `rogue_config.json` - Configuration for enabled plugins
- `plugins/` - Directory containing all game plugins
- `games/` - Directory containing game data (maps, pieces, etc.)

## Setup

1. Make sure you have Python 3 installed
2. Run the game with: `python3 main.py`