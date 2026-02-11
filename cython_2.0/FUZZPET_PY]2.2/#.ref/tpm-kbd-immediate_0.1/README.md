# TPM Keyboard-First Menu System

This project demonstrates the True Piece Method (TPM) architecture for menu systems with direct keyboard pipeline integration, using the same input processing pipeline as the game control system.

## Architecture Overview

The system implements TPM architecture where:
- Every component is a "piece" stored as human-readable text files
- All state is persistent in piece files under `games/default/pieces/`
- All keyboard input flows through `games/default/pieces/keyboard/history.txt`
- Plugins communicate via system() calls and file-based piece storage
- Input processing follows the same pipeline as the original kilorb.c game

## Components

### Core Architecture
- **Piece Manager**: Discovers and manages all pieces in `games/default/pieces/`
- **Keyboard Handler**: Logs all keyboard input to history file following kilorb.c standards (1000=ARROW_UP, 1001=ARROW_DOWN, etc.)
- **Control Handler**: Polls history file and translates key codes to menu actions
- **Display Handler**: Renders menu system based on current piece states
- **Event Manager**: Handles scheduled events and piece state changes

### Plugin Structure
- `plugins/core/` - Core functionality: piece_manager, keyboard, control, event_manager, methods
- `plugins/ui/` - UI functionality: menu display
- Each plugin follows PluginInterface pattern for standardized interaction

### Piece Hierarchy
All state stored in `games/default/pieces/`:
- `keyboard/` - Input logging and management
- `player1/` - Player state and position
- `world/` - World map and entities
- `menu/` - Menu structures and selection states
- `clock/` - Time and turn management
- `display/` - Display refresh state

## Input Pipeline

The keyboard input follows the same processing chain as the original game:
1. User presses key
2. Keyboard plugin logs to `games/default/pieces/keyboard/history.txt` with numeric codes
3. Control plugin polls history file for new entries
4. Key codes translated using kilorb.c enum values (WASD=character codes, Arrows=1000+)
5. Actions executed based on current menu state in pieces/
6. Piece state updated and saved to appropriate file
7. Display refreshed based on new piece states

## Features

- **Immediate Input Echo**: Keyboard input is echoed to screen immediately when pressed
- **Piece-Based Menus**: All menus and submenus implemented as individual pieces
- **Direct Access**: External programs can modify piece files directly to change game state
- **Event System**: Event-driven architecture with piece listeners and method execution
- **Modular Design**: Each component independent and swappable via plugin system

## Menu System

Implemented as multiple interconnected pieces:
- Main menu with submenus (0-4 options)
- Navigation with number keys, arrow keys, WASD
- Menu state persisted in piece files
- Dynamic menu updates through event system

## Usage

Run the system:
```bash
python3 menu_system.py
```

Controls:
- Number keys (0-4) for menu options
- WASD or arrow keys for navigation
- Menu items update piece states which trigger display updates

## TPM Compliance

- ✅ All state in human-readable text files
- ✅ Each component is a "piece"
- ✅ Loose coupling through event system
- ✅ Shared file system as communication bus
- ✅ Extensible through plugin system
- ✅ Direct state manipulation possible via external programs

## File Structure

```
tpm-kbd-immediate/
├── menu_system.py          # Main entry point
├── main_menu_engine.py     # Engine coordinating plugins
├── plugin_interface.py     # Base plugin interface
├── plugins/
│   ├── core/               # Core plugins (piece_manager, keyboard, control, etc.)
│   └── ui/                 # UI plugins (display)
└── games/default/pieces/   # All piece state files
    ├── keyboard/
    ├── player1/
    ├── world/
    ├── menu/
    ├── clock/
    └── display/
```

This system demonstrates how the same TPM architecture used for games can be applied to menu systems while maintaining the same reliable input processing pipeline and piece-based state management.