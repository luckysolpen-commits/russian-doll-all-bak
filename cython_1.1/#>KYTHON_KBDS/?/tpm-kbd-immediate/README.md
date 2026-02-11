# TPM Keyboard Menu System

This project implements a TPM (True Piece Method)-compliant menu system that uses the same plugin architecture as the original game, but adapted for menu navigation instead of game functionality.

## Architecture

The system maintains the same plugin structure as the original game:

- **Piece Manager Plugin**: Manages piece discovery, loading, and coordination
- **Keyboard Plugin**: Handles keyboard input logging to history file following kilorb.c standards
- **Menu Control Plugin**: Handles keyboard input (arrows, numbers) and translates to menu events
- **Menu Display Plugin**: Handles rendering of menu interface
- **Event Manager Plugin**: Handles scheduled events based on clock time

## Key Features

1. **TPM Compliance**: All components follow the True Piece Method architecture where state is stored in human-readable text files
2. **Same Input Pipeline**: Uses the same keyboard input processing pipeline as the control plugin
3. **Piece-Based Menus**: Each menu is implemented as a piece with its own state
4. **Immediate Input Echo**: Keyboard input is echoed immediately when pressed
5. **Navigation Support**: Number keys for selection, arrow keys for navigation

## File Structure

- `games/default/pieces/` - Contains all menu pieces as text files
- `menu_system.py` - Main entry point
- `main_menu_engine.py` - Main engine coordinating plugins
- `menu_config.json` - Configuration file for plugin loading
- `plugins/core/` - Core plugins directory
  - `menu_control.py` - Handles menu navigation (adapted from control plugin)
  - `piece_manager.py` - Manages menu pieces (same as game)
  - `keyboard.py` - Handles input logging (same as game)
  - `event_manager.py` - Handles events (same as game)
- `plugins/ui/` - UI plugins directory
  - `menu_display.py` - Handles menu display (adapted from display plugin)

## Usage

Run the system with:
```bash
python3 menu_system.py
```

Use number keys to select menu options, arrow keys to navigate, or 'q' to quit.

## Components

Each plugin follows the same interface and patterns as the original game plugins but adapted for menu functionality:

- `menu_control_plugin.py` - Handles menu navigation (adapted from control plugin)
- `menu_display_plugin.py` - Handles menu display (adapted from display plugin)
- `piece_manager_plugin.py` - Manages menu pieces (same as game)
- `keyboard_plugin.py` - Handles input logging (same as game)
- `event_manager_plugin.py` - Handles events (same as game)

This demonstrates that any application can be built using the TPM architecture while maintaining the same reliable input processing pipeline.