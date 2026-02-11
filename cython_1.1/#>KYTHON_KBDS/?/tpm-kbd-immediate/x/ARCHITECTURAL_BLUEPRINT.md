# TPM Menu System Architecture Blueprint

## Overview
This system demonstrates the True Piece Method (TPM) architecture applied to a menu system with keyboard input processing. The architecture maintains the same input pipeline as the original game control plugin but adapts it for menu navigation instead of game-specific actions.

## Core Architecture Nodes

### 1. Plugin Interface (`plugin_interface.py`)
- **Functionality**: Base interface that all plugins must implement
- **TPM Adherence**: Provides standardized interface for all plugins
- **Methods**: initialize(), handle_input(), update(), render(), activate(), deactivate()
- **Improvement Ideas**: Could add more specific method contracts for better type safety

### 2. Plugin Manager (`main_menu_engine.py`)
- **Functionality**: Coordinates loading, initialization, and execution of plugins
- **TPM Adherence**: Centralized coordination without controlling business logic
- **Features**: Dynamic plugin loading, method forwarding, capability discovery
- **Improvement Ideas**: Could implement hot-reloading, better error isolation

### 3. Piece Manager Plugin (`plugins/core/piece_manager.py`)
- **Functionality**: Discovers and manages all piece state files
- **TPM Adherence**: **Excellent** - Core TPM principle of state in human-readable files
- **Features**: Auto-discovers pieces in `games/default/pieces/`, loads state from .txt files, persists state changes
- **Improvement Ideas**: Add file change monitoring for live updates

### 4. Keyboard Plugin (`plugins/core/keyboard.py`)
- **Functionality**: Logs all keyboard input to history file following kilorb.c standards
- **TPM Adherence**: Maintains the same input processing pipeline as original game
- **Features**: Maps key codes to kilorb.c enum values, handles escape sequences, polls file for new inputs
- **Improvement Ideas**: Add more comprehensive key mapping, modifier key support

### 5. Menu Control Plugin (`plugins/core/menu_control.py`)
- **Functionality**: Polls keyboard history file and translates key codes to menu actions
- **TPM Adherence**: **Excellent** - Maintains same pipeline as control plugin but for menus
- **Features**: Number key selection, arrow key navigation, menu state management, submenu navigation
- **Improvement Ideas**: Add key binding customization, macro support

### 6. Menu Display Plugin (`plugins/ui/menu_display.py`)
- **Functionality**: Renders menu interface based on current piece states
- **TPM Adherence**: Relies entirely on piece state for rendering
- **Features**: Shows current menu with selection indicator, displays controls
- **Improvement Ideas**: Add visual theming capabilities, multi-window support

### 7. Event Manager Plugin (`plugins/core/event_manager.py`)
- **Functionality**: Handles scheduled events and triggers responses for event listeners
- **TPM Adherence**: Enables event-driven architecture within TPM
- **Features**: Time-based event execution, event listener triggering
- **Improvement Ideas**: Add more sophisticated scheduling, dependency management

## Data Flow Architecture

### Input Processing Pipeline
1. Keyboard input → `keyboard.py` logs to `games/default/pieces/keyboard/history.txt`
2. `menu_control.py` polls history file for new key codes (same as game control)
3. Key codes translated using kilorb.c enum values (1000=ARROW_UP, 1001=ARROW_DOWN, etc.)
4. Actions executed based on current menu state in piece files
5. Menu state updated in corresponding piece file
6. Display refresh triggered through piece state changes

### State Persistence
- All state stored in human-readable `.txt` files under `games/default/pieces/`
- Each piece directory contains `{piece_name}.txt` with key-value pairs
- State changes automatically persisted to disk
- External programs can modify state files directly

### Event System
- Pieces can declare `event_listeners` in their state files
- When events occur, event manager triggers corresponding methods
- Response methods execute using piece state and update as needed

## TPM Adherence Assessment

### Strengths
✅ **Everything is a Piece**: Menus, keyboard, control state, display all stored as pieces
✅ **State in Text Files**: Human-readable, persistent, auditable
✅ **Event-Driven**: Loose coupling between components using events
✅ **Modular**: Components can be added/removed independently
✅ **Same Pipeline**: Uses identical input processing as original game
✅ **File-Based**: External programs can modify state files directly

### Areas for Improvement
⚠️ **Type Consistency**: String vs integer coercion could be more robust
⚠️ **Error Handling**: Better isolation when plugins fail
⚠️ **Performance**: File I/O on every state change could be optimized
⚠️ **Validation**: State validation for piece integrity

## Future Enhancements

### For Current System
1. Add file change monitoring for live updates
2. Implement plugin versioning and compatibility checking
3. Add more sophisticated key binding systems
4. Optimize file I/O with caching layers
5. Add plugin dependency management

### For Extension to Roguelike
1. Add world/map rendering plugin
2. Implement player movement system
3. Create NPC behavior system
4. Add inventory and combat systems
5. Integrate map and menu display in unified interface

## Roadmap to ASCII Roguelike

### Phase 1: Core Integration
- Create world rendering plugin that displays ASCII maps
- Adapt menu system for in-game actions
- Integrate keyboard controls for both movement and menu activation
- Use existing map files from `games/default/maps/`

### Phase 2: Game Mechanics
- Implement player movement with collision detection
- Create NPC AI that responds to player actions
- Add turn-based system and state management
- Connect inventory and action systems

### Phase 3: Advanced Features
- Add combat and skill systems
- Implement save/load functionality
- Create procedural map generation
- Add multiplayer support

## Conclusion

This architecture demonstrates the power and flexibility of True Piece Method. The same plugin system used for the game can be adapted for menu systems, text editors, or any other application. The system maintains 100% compatibility with the original input processing pipeline while providing a clean separation of concerns and complete state transparency through human-readable text files.