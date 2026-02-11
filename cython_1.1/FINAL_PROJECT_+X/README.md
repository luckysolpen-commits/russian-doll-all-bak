# TPM Orchestration System - FINAL PROJECT_+X

## Overview

This system implements the same TPM architecture as the previous version but using true orchestration with independent executable modules that communicate via system() calls and file-based piece storage, rather than in-process function calls.

## Architecture

### TPM Compliance
- **Pieces**: All state stored in human-readable text files under `peices/` directory
- **Independent Modules**: Each module is a separate executable in `+x/` directory
- **Orchestration**: Communication via system() calls and file I/O instead of function linking
- **True Decoupling**: Each module can be developed, tested, and deployed independently

### Module Structure
- `+x/render_map.+x` - Renders the ASCII map by combining world map, entities, and player position
- `+x/get_player_pos.+x` - Retrieves player coordinates from piece file
- `+x/update_player_pos.+x` - Updates player coordinates in piece file
- `orchestrator.exe` - Main controller that calls modules via system() calls

### File-Based Communication
- State stored in `peices/` directory following TPM architecture:
  - `peices/world/map.txt` - World map data
  - `peices/player/player.txt` - Player state
  - `peices/keyboard/keyboard.txt` - Keyboard input piece
  - `peices/keyboard/history.txt` - Keyboard input history (main pipeline file)
  - `peices/clock/clock.txt` - Game clock/time
  - `peices/menu/menu.txt` - Menu options and selections
  - `peices/zombie/zombie.txt` - NPC state
  - `peices/sheep/sheep.txt` - Animal state
- All modules communicate through piece files
- All keyboard input flows through `peices/keyboard/history.txt` following the TPM input pipeline
- Each module operates independently and communicates through the shared file system

## Key Features

### True Module Independence
- Each executable module can run independently
- No shared memory or function linking between modules
- Each module can be developed with different languages/technologies
- Failure of one module doesn't crash the system (graceful degradation)

### Same TPM Pipeline
- Maintains the same functionality as the previous integrated version
- All TPM principles preserved (pieces, states, events)
- Same game mechanics and controls
- Same file-based persistence

### Orchestration Benefits
- **Scalability**: Modules can run on different machines/processes
- **Maintainability**: Each module encapsulated separately
- **Debugging**: Individual modules can be tested standalone
- **Deployment**: Modules can be updated independently
- **Technology Flexibility**: Different modules can use different tech stacks

## Visual Standards

The system now properly displays:
- Combined world map with entities overlaid 
- Player position with '@' symbol at correct coordinates
- Clear map boundaries and legend
- Clean menu and control information
- Proper alignment and formatting using \r for cleaner updates

## Usage

All modules are compiled C programs with .+x extension as per your specification:
- `./orchestrator.exe` - Main game loop
- `./+x/render_map.+x` - Display rendering
- `./+x/get_player_pos.+x` - State access
- `./+x/update_player_pos.+x` - State modification

## Technical Implementation

### Communication Pattern
1. Orchestrator calls `get_player_pos.+x` to retrieve coordinates
2. Orchestrator calls `update_player_pos.+x` to update state after movement
3. Orchestrator calls `render_map.+x` to display current world state
4. `render_map.+x` combines data from all piece files for unified display
5. All state changes persisted to appropriate piece files

### TPM Adherence
- All state in human-readable text files
- Modular architecture with loose coupling
- Event-driven through file changes
- Persistent and auditable state
- External programs can modify state directly

This represents a true orchestration architecture where each module is a separate executable that communicates through system calls and file-based piece storage, achieving the same functionality as the integrated version but with stronger module independence.