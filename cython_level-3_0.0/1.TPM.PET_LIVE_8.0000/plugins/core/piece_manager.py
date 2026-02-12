"""
Piece Manager plugin for the Pet Philosophy project
Manages piece discovery, loading, and coordination following TPM principles
"""

import os
from plugin_interface import PluginInterface


class Plugin(PluginInterface):
    def __init__(self, game_engine):
        super().__init__(game_engine)
        self.pieces = {}
        self.piece_directory = "pieces/"
        
    def initialize(self):
        """Initialize by discovering and loading all pieces"""
        print("Piece Manager plugin initialized")
        self.discover_pieces()
        print(f"Loaded {len(self.pieces)} pieces")
    
    def discover_pieces(self):
        """Discover and load all pieces from the pieces directory"""
        if not os.path.exists(self.piece_directory):
            os.makedirs(self.piece_directory, exist_ok=True)
            print(f"Created piece directory: {self.piece_directory}")
            return
        
        # Discover all piece directories
        for piece_name in os.listdir(self.piece_directory):
            piece_path = os.path.join(self.piece_directory, piece_name)
            if os.path.isdir(piece_path):
                # Look for the piece file (should be named the same as the directory)
                # Special case: the fuzzpet piece is named "piece.txt" instead of "fuzzpet.txt"
                if piece_name == "fuzzpet":
                    piece_file = os.path.join(piece_path, "piece.txt")
                else:
                    piece_file = os.path.join(piece_path, f"{piece_name}.txt")
                
                if os.path.exists(piece_file):
                    self.pieces[piece_name] = self.load_piece_from_file(piece_file)
                    print(f"Loaded piece: {piece_name}")
                else:
                    # Create basic piece file if it doesn't exist
                    self.create_basic_piece_file(piece_path, piece_name)
                    if piece_name == "fuzzpet":
                        piece_file = os.path.join(piece_path, "piece.txt")
                    else:
                        piece_file = os.path.join(piece_path, f"{piece_name}.txt")
                    self.pieces[piece_name] = self.load_piece_from_file(piece_file)
                    print(f"Created basic piece: {piece_name}")
    
    def create_basic_piece_file(self, piece_path, piece_name):
        """Create a basic piece file if it doesn't exist"""
        piece_file = os.path.join(piece_path, f"{piece_name}.txt")
        if piece_name == "keyboard":
            with open(piece_file, 'w') as f:
                f.write("""session_id 12345
current_key_log_path pieces/keyboard/history.txt
logging_enabled true
keys_recorded 0
last_key_logged 
last_key_time 
last_key_code 0
status active
event_listeners [key_press]
methods [log_key, reset_session, get_session_log, enable_logging, disable_logging]
""")
        elif piece_name == "master_ledger":
            with open(piece_file, 'w') as f:
                f.write("""status active
last_entry_id 0
total_entries 0
event_listeners [system_event, method_call, state_change]
methods [log_event, read_log, search_log, clear_log]
""")
        elif piece_name == "mover":
            with open(piece_file, 'w') as f:
                f.write("""position_x 0
position_y 0
last_direction none
status active
event_listeners [key_w, key_a, key_s, key_d]
methods [move_up, move_down, move_left, move_right, get_position]
responses:
  key_w: Moving up
  key_a: Moving left  
  key_s: Moving down
  key_d: Moving right
""")
        elif piece_name == "command_processor":
            with open(piece_file, 'w') as f:
                f.write("""current_command 
command_buffer 
command_mode waiting
status active
event_listeners [key_press, command_entered]
methods [process_command, clear_buffer, execute_command]
responses:
  key_press: Key pressed
  command_entered: Command processed
""")
    
    def load_piece_from_file(self, file_path):
        """Load a piece's state from its text file"""
        piece_state = {}
        try:
            with open(file_path, 'r') as f:
                lines = f.readlines()
        
            # First pass: identify all sections and their boundaries
            sections = {}  # {line_index: section_name}
            for i, line in enumerate(lines):
                stripped = line.strip()
                if stripped.endswith(':') and not stripped.startswith('#') and not ' ' in stripped:
                    sections[i] = stripped[:-1].lower()
        
            # Second pass: parse content
            current_section = None
            section_start = -1
            
            for i, line in enumerate(lines):
                stripped = line.strip()
                
                if stripped.startswith('#') or not stripped:
                    continue
                    
                # Check if this line starts a new section
                if i in sections:
                    current_section = sections[i]
                    # Initialize the section dictionary if it doesn't exist
                    if current_section not in piece_state:
                        if current_section in ['state', 'responses']:
                            piece_state[current_section] = {}
                        elif current_section in ['methods', 'event_listeners']:
                            piece_state[current_section] = []
                    continue
                
                # Handle top-level methods/event_listeners (not in any section)
                if stripped.startswith('methods:') or stripped.startswith('event_listeners:'):
                    if stripped.startswith('methods:'):
                        value = stripped[len('methods:'):].strip()
                        items = [item.strip() for item in value.split(',') if item.strip()]
                        piece_state['methods'] = items
                    elif stripped.startswith('event_listeners:'):
                        value = stripped[len('event_listeners:'):].strip()
                        items = [item.strip() for item in value.split(',') if item.strip()]
                        piece_state['event_listeners'] = items
                # Handle content within sections
                elif current_section and ':' in stripped:
                    key, value = stripped.split(':', 1)
                    key = key.strip()
                    value = value.strip()
                    
                    if current_section in ['state', 'responses']:
                        piece_state[current_section][key] = value
                    elif current_section in ['methods', 'event_listeners']:
                        items = [item.strip() for item in value.split(',') if item.strip()]
                        if current_section not in piece_state:
                            piece_state[current_section] = []
                        piece_state[current_section].extend(items)
                elif ':' in stripped and current_section is None and not stripped.startswith('#'):
                    # Handle key:value pairs outside sections
                    parts = stripped.split(':', 1)
                    if len(parts) == 2:
                        key, value = parts[0].strip(), parts[1].strip()
                        # Try to convert numbers where appropriate
                        if key in ['keys_recorded', 'last_key_code', 'last_entry_id', 'total_entries', 'position_x', 'position_y', 'hunger', 'happiness', 'energy', 'level']:
                            try:
                                value = int(value)
                            except ValueError:
                                try:
                                    value = float(value)
                                except ValueError:
                                    pass  # Keep as string if not a valid number
                        piece_state[key] = value
        except Exception as e:
            print(f"Error loading piece from {file_path}: {e}")
        
        return piece_state
    
    def get_piece_state(self, piece_name):
        """Get the current state of a piece"""
        return self.pieces.get(piece_name, None)
    
    def update_piece_state(self, piece_name, new_state):
        """Update the state of a piece and save it to file"""
        self.pieces[piece_name] = new_state

        # Save to file - adjust file name for fuzzpet
        if piece_name == "fuzzpet":
            piece_path = os.path.join(self.piece_directory, piece_name, "piece.txt")
        else:
            piece_path = os.path.join(self.piece_directory, piece_name, f"{piece_name}.txt")
        
        try:
            with open(piece_path, 'w') as f:
                for key, value in new_state.items():
                    if key in ['state', 'responses'] and isinstance(value, dict):
                        # Handle section writing
                        f.write(f"{key}:\n")
                        for k, v in value.items():
                            f.write(f"  {k}: {v}\n")
                    elif key in ['methods', 'event_listeners'] and isinstance(value, list):
                        # Handle lists
                        f.write(f"{key}: {', '.join(value)}\n")
                    elif key == '# Piece' and isinstance(value, str):
                        # Handle special piece identifier
                        f.write(f"# {key}: {value}\n")
                    else:
                        # Handle regular key-value pairs
                        f.write(f"{key}: {value}\n")
        except Exception as e:
            print(f"Error saving piece {piece_name}: {e}")