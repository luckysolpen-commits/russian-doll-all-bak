# TPM C Keyboard System

This is a True Piece Method (TPM) compliant keyboard input system implemented in C using modular components.

## Architecture

The system follows the True Piece Method architecture:
- **Pieces**: Stored in text files under `peices/` directory
- **Methods**: Implemented as functions in separate C modules
- **Events**: Handled by the event manager module
- **Plugins**: Each .c file acts as a plugin providing specific functionality

## Directory Structure

```
tpm_c_kbd/
├── keyboard_input.c      # Keyboard input handling module
├── display.c            # Display rendering module
├── piece_manager.c      # Piece management module
├── event_manager.c      # Event handling module
├── main.c              # Full keyboard system (requires terminal)
├── history_monitor.c   # History monitoring system
├── keyboard_simulator.c # Input simulation for testing
├── compile.sh          # Compilation script
├── bin/                # Compiled executables
└── peices/             # Piece files (TPM data storage)
    └── input/
        └── keyboard/
            ├── keyboard.txt  # Keyboard piece definition
            └── history.txt   # Input history log
```

## Features

1. **Silent Input**: Keys are captured and written to history.txt without terminal echo
2. **History-Based Display**: System reads from history.txt to simulate typing
3. **External Input Compatible**: Alternative input methods can write directly to history.txt
4. **Modular Architecture**: Each component is in its own C file
5. **No Headers/Linking**: Using direct inclusion for modularity as per requirements

## Compilation

```bash
./compile.sh
```

This creates three executables in the `bin/` directory:
- `tpm_keyboard`: Full system with live keyboard input (needs real terminal)
- `history_monitor`: Watches history.txt for changes and displays content
- `keyboard_simulator`: Simulates keyboard input for testing

## Usage

### For live keyboard input (requires terminal):
```bash
./bin/tpm_keyboard
```

### For monitoring history changes:
```bash
./bin/history_monitor
```

### For simulating keyboard input:
```bash
./bin/keyboard_simulator
```

## TPM Compliance

This implementation follows TPM principles:
- All state stored in human-readable text files
- Modular components that can be independently maintained
- Clear separation between data (pieces) and logic (methods)
- Event-driven architecture for reactive behavior
- Support for external input sources through history file

## Working Mechanism

1. Keyboard input is captured and written to `peices/input/keyboard/history.txt`
2. The display system reads from history.txt to reconstruct the typing experience
3. Piece files define the structure and configuration of the system
4. Events can be triggered when piece state changes
5. Alternative input methods can write directly to history.txt to simulate typing