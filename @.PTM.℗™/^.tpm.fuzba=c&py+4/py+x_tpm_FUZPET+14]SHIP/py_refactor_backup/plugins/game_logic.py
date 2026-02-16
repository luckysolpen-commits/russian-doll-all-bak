# This piece is the main game logic.
# It reads keyboard input, updates state, and writes the display frame.

import os
import sys
import time
import threading

# --- File Paths ---
script_dir = os.path.dirname(__file__)
KEYBOARD_HISTORY_PATH = os.path.join(script_dir, '../pieces/keyboard/history.txt')
CURRENT_FRAME_PATH = os.path.join(script_dir, '../pieces/display/current_frame.txt')
CURRENT_FRAME_TMP_PATH = os.path.join(script_dir, '../pieces/display/current_frame.txt.tmp')
MAP_PATH = os.path.join(script_dir, '../pieces/world/map.txt')
MASTER_LEDGER_PATH = os.path.join(script_dir, '../pieces/master_ledger/ledger.txt')

# --- Game State ---

# --- Game State ---
# Simplified state, managed internally for now.
pet_state = {
    'pos_x': 5,
    'pos_y': 5,
    'name': 'FuzzPet',
    'hunger': 50,
    'happiness': 50,
    'energy': 50,
    'level': 1,
}
last_command_output = ["[GAME] Game logic started."]
map_grid = []

def load_map():
    """Loads the map from file or creates a default one."""
    global map_grid
    if not os.path.exists(MAP_PATH):
        print("Map file not found, creating a default one.", file=sys.stderr)
        os.makedirs(os.path.dirname(MAP_PATH), exist_ok=True)
        default_map = [
            "##########",
            "#        #",
            "#        #",
            "#        #",
            "#   @    #",
            "#        #",
            "#        #",
            "#        #",
            "#        #",
            "##########",
        ]
        with open(MAP_PATH, 'w') as f:
            f.write("\n".join(default_map))

    with open(MAP_PATH, 'r') as f:
        map_grid = [list(line.strip()) for line in f.readlines()]

def render_frame():
    """Generates the display frame string and writes it to file."""
    global last_command_output
    
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

    # Pet Status
    frame_content.append(f"Pet Status: Name: {pet_state['name']}, Hunger: {pet_state['hunger']}, "
                       f"Happiness: {pet_state['happiness']}, Energy: {pet_state['energy']}, "
                       f"Level: {pet_state['level']}")
    
    # Command output
    if last_command_output:
        frame_content.append("")
        frame_content.extend(last_command_output)
        last_command_output = [] # Clear after showing

    # Commands
    pet_commands = "Pet Cmds: 1[Feed]|2[Play]|3[Sleep]|4[Status]|5[Evolve]"
    movement_commands = "Movement: W↑|A←|S↓|D→"
    frame_content.append(f"\n{movement_commands} | {pet_commands}")

    # Write to a temporary file first
    os.makedirs(os.path.dirname(CURRENT_FRAME_TMP_PATH), exist_ok=True)
    with open(CURRENT_FRAME_TMP_PATH, 'w') as f:
        f.write("\n".join(frame_content))
    
    # Atomically rename the temporary file to the final file
    os.rename(CURRENT_FRAME_TMP_PATH, CURRENT_FRAME_PATH)

def process_key(key_code):
    """Updates game state based on key code."""
    global last_command_output
    char = chr(key_code)
    
    # Mover logic
    if char == 'w':
        log_event_to_master_ledger('MovementEvent', f'Move Up (key: {char})', 'keyboard_input')
        pet_state['pos_y'] = max(0, pet_state['pos_y'] - 1)
    elif char == 's':
        log_event_to_master_ledger('MovementEvent', f'Move Down (key: {char})', 'keyboard_input')
        pet_state['pos_y'] += 1
    elif char == 'a':
        log_event_to_master_ledger('MovementEvent', f'Move Left (key: {char})', 'keyboard_input')
        pet_state['pos_x'] = max(0, pet_state['pos_x'] - 1)
    elif char == 'd':
        log_event_to_master_ledger('MovementEvent', f'Move Right (key: {char})', 'keyboard_input')
        pet_state['pos_x'] += 1
        
    # Command processor logic
    elif char == '1': # Feed
        log_event_to_master_ledger('CommandExecuted', 'Feed pet', 'keyboard_input')
        pet_state['hunger'] = max(0, pet_state['hunger'] - 20)
        last_command_output = ["[CMD] Fed the pet. Hunger decreased."]
    elif char == '2': # Play
        log_event_to_master_ledger('CommandExecuted', 'Play with pet', 'keyboard_input')
        pet_state['happiness'] = min(100, pet_state['happiness'] + 15)
        last_command_output = ["[CMD] Played with the pet. Happiness increased."]
    elif char == '3': # Sleep
        log_event_to_master_ledger('CommandExecuted', 'Pet sleeps', 'keyboard_input')
        pet_state['energy'] = min(100, pet_state['energy'] + 20)
        last_command_output = ["[CMD] Pet is sleeping. Energy increased."]
    elif char == '4': # Status
        log_event_to_master_ledger('CommandExecuted', 'Check pet status', 'keyboard_input')
        last_command_output = ["[CMD] Displaying current pet status."]
    elif char == '5': # Evolve
        log_event_to_master_ledger('CommandExecuted', 'Evolve pet', 'keyboard_input')
        pet_state['level'] += 1
        last_command_output = ["[CMD] Pet has evolved! Level up!"]

    # Log the resulting state change
    log_event_to_master_ledger('StateUpdate', f'Pet state updated: {pet_state}', 'game_logic')

    # Always re-render after processing a key
    render_frame()

def watch_history_file():
    """Tails the keyboard history file for new inputs."""
    print("Game logic started. Watching for keyboard input.", file=sys.stderr)
    
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
    """Logs an event to the master ledger file."""
    timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
    log_entry = f"[{timestamp}] {event_type}: {description} | Source: {source}\n"
    os.makedirs(os.path.dirname(MASTER_LEDGER_PATH), exist_ok=True) # Ensure dir exists for ledger
    with open(MASTER_LEDGER_PATH, 'a') as ledger:
        ledger.write(log_entry)

def main():
    """Main function."""
    print("Starting game logic piece.", file=sys.stderr)
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
