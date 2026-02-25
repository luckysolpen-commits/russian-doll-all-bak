# This piece is the main game logic.
# It reads keyboard input, updates state, and writes the display frame.

import os
import sys
import time
import threading
import subprocess

# Import the FuzzPet and PieceManager functionality
# Add piece plugin directories to path for imports
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '../../fuzzpet/plugins'))
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '../../master_ledger/plugins'))

from fuzzpet import FuzzPet
from piece_manager import piece_manager

# --- File Paths ---
script_dir = os.path.dirname(__file__)
# From pieces/world/plugins/, go up 2 levels to project root, then into pieces/
PIECES_ROOT = os.path.abspath(os.path.join(script_dir, '../../'))
KEYBOARD_HISTORY_PATH = os.path.join(PIECES_ROOT, 'keyboard/history.txt')
CURRENT_FRAME_PATH = os.path.join(PIECES_ROOT, 'display/current_frame.txt')
CURRENT_FRAME_TMP_PATH = os.path.join(PIECES_ROOT, 'display/current_frame.txt.tmp')
MAP_PATH = os.path.join(PIECES_ROOT, 'world/map.txt')
MASTER_LEDGER_PATH = os.path.join(PIECES_ROOT, 'master_ledger/ledger.txt')

# --- Game State ---
last_command_output = ["[GAME] Game logic started."]
map_grid = []

def load_map():
    """Loads the map from file or creates a default one."""
    global map_grid
    map_grid = piece_manager.load_map()

def render_frame():
    """Generates the display frame string and writes it to file."""
    global last_command_output, fuzzpet
    
    # Get the current pet state from the FuzzPet instance
    pet_state = fuzzpet.get_state()
    
    # Get turn and time from state.txt
    turn_str = "0"
    time_str = "08:00:00"
    
    try:
        with open("pieces/clock/state.txt", "r") as f:
            lines = f.readlines()
            for line in lines:
                line = line.strip()
                if line.startswith("turn "):
                    turn_str = line[5:]  # Get after "turn "
                elif line.startswith("time "):
                    time_str = line[5:]  # Get after "time "
    except FileNotFoundError:
        # If file doesn't exist, create it with default values
        with open("pieces/clock/state.txt", "w") as f:
            f.write("turn 0\ntime 08:00:00\n")
    
    # Create a copy of the map to draw on
    display_grid = [row[:] for row in map_grid]
    
    # Place pet on map
    if 0 <= pet_state['pos_y'] < len(display_grid) and 0 <= pet_state['pos_x'] < len(display_grid[0]):
        if display_grid[pet_state['pos_y']][pet_state['pos_x']] == ' ':
            display_grid[pet_state['pos_y']][pet_state['pos_x']] = '@'

    # Build the frame content
    frame_content = []
    frame_content.append("="*60)
    frame_content.append("PET POSITION MAP:")
    frame_content.append("="*60)
    for row in display_grid:
        frame_content.append("".join(row))
    frame_content.append(f"Pet Position: ({pet_state['pos_x']}, {pet_state['pos_y']})")
    frame_content.append("="*60)

    # Pet Status with Turn and Time
    frame_content.append(fuzzpet.get_status_message())
    frame_content.append(f"Turn: {turn_str}")
    frame_content.append(f"Time: {time_str}")
    
    # Command output
    if last_command_output:
        frame_content.append("")
        frame_content.extend(last_command_output)
        last_command_output = [] # Clear after showing

    # Commands
    pet_commands = "Pet Cmds: 1[Feed]|2[Play]|3[Sleep]|4[Status]|5[Evolve]|6[End Turn]"
    movement_commands = "Movement: W↑|A←|S↓|D→"
    frame_content.append(f"\n{movement_commands} | {pet_commands}")

    # Save the frame using PieceManager
    piece_manager.save_frame("\n".join(frame_content))
    
    # Append marker to signal frame change (for reliable detection by both renderers)
    marker_path = os.path.join(script_dir, '../../master_ledger/frame_changed.txt')
    os.makedirs(os.path.dirname(marker_path), exist_ok=True)
    with open(marker_path, 'a') as marker:
        marker.write("X\n")

def process_key(key_code):
    """Updates game state based on key code."""
    global last_command_output, fuzzpet
    char = chr(key_code)
    
    # Mover logic
    if char == 'w':
        log_event_to_master_ledger('MovementEvent', f'Move Up (key: {char})', 'keyboard_input')
        fuzzpet.update_position('up')
        # Increment both time and turn on movement
        subprocess.run(['python3', '../../clock/plugins/increment_time.py'], check=False)
        subprocess.run(['python3', '../../clock/plugins/increment_turn.py'], check=False)
    elif char == 's':
        log_event_to_master_ledger('MovementEvent', f'Move Down (key: {char})', 'keyboard_input')
        fuzzpet.update_position('down')
        # Increment both time and turn on movement
        subprocess.run(['python3', '../../clock/plugins/increment_time.py'], check=False)
        subprocess.run(['python3', '../../clock/plugins/increment_turn.py'], check=False)
    elif char == 'a':
        log_event_to_master_ledger('MovementEvent', f'Move Left (key: {char})', 'keyboard_input')
        fuzzpet.update_position('left')
        # Increment both time and turn on movement
        subprocess.run(['python3', '../../clock/plugins/increment_time.py'], check=False)
        subprocess.run(['python3', '../../clock/plugins/increment_turn.py'], check=False)
    elif char == 'd':
        log_event_to_master_ledger('MovementEvent', f'Move Right (key: {char})', 'keyboard_input')
        fuzzpet.update_position('right')
        # Increment both time and turn on movement
        subprocess.run(['python3', '../../clock/plugins/increment_time.py'], check=False)
        subprocess.run(['python3', '../../clock/plugins/increment_turn.py'], check=False)
        
    # Command processor logic - delegate to fuzzpet
    elif char in ['1', '2', '3', '4', '5']: # FuzzPet commands
        cmd_result = fuzzpet.process_command(char)
        log_event_to_master_ledger('CommandExecuted', f'Pet command {char} processed', 'keyboard_input')
        last_command_output = [cmd_result]
    elif char == '6':
        # End turn command - increment both time and turn
        log_event_to_master_ledger('EventFire', f'Turn end on clock (key: {char})', 'game_logic')
        last_command_output = ["[CMD] Turn ended. Time and turn advanced."]
        
        # Execute end turn (increments both time and turn)
        subprocess.run(['python3', '../../clock/plugins/end_turn.py'], check=False)
    else:
        # Handle other commands if necessary
        pass

    # Log the resulting state change
    current_state = fuzzpet.get_state()
    log_event_to_master_ledger('StateUpdate', f'Pet state updated: {current_state}', 'game_logic')

    # Always re-render after processing a key
    render_frame()

def watch_history_file():
    """Tails the keyboard history file for new inputs."""
    global map_grid
    print("Game logic started. Watching for keyboard input.", file=sys.stderr)
    
    # Ensure the keyboard history file exists
    os.makedirs(os.path.dirname(KEYBOARD_HISTORY_PATH), exist_ok=True)
    
    # Go to the end of the file
    if os.path.exists(KEYBOARD_HISTORY_PATH):
        with open(KEYBOARD_HISTORY_PATH, 'r') as f:
            f.seek(0, 2) # Go to the end of the file
            while True:
                line = f.readline()
                if not line:
                    time.sleep(0.1) # Sleep briefly
                    continue
                
                try:
                    key_code = int(line.strip())
                    process_key(key_code)
                except ValueError:
                    print(f"Warning: Could not parse line '{line.strip()}' in history file.", file=sys.stderr)
    else:
        print(f"Warning: History file not found at {KEYBOARD_HISTORY_PATH}. Waiting...", file=sys.stderr)
        while not os.path.exists(KEYBOARD_HISTORY_PATH):
            time.sleep(1)
        watch_history_file() # Recurse to start watching

def log_event_to_master_ledger(event_type, description, source='game_logic'):
    """Logs an event to the master ledger file using PieceManager."""
    piece_manager.append_to_master_ledger(event_type, description, source)

def main():
    """Main function."""
    print("Starting game logic piece.", file=sys.stderr)
    global fuzzpet
    
    # Initialize the FuzzPet instance
    fuzzpet = FuzzPet()
    
    os.makedirs(os.path.dirname(MASTER_LEDGER_PATH), exist_ok=True) # Ensure master ledger directory exists
    load_map()
    render_frame() # Initial render
    
    try:
        watch_history_file()
    except KeyboardInterrupt:
        print("\nGame logic piece stopped.", file=sys.stderr)
    except Exception as e:
        print(f"\nAn error occurred in game logic: {e}", file=sys.stderr)

if __name__ == "__main__":
    main()
