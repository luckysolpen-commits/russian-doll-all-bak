"""
Piece Manager plugin - Same as the game's piece manager but for menu system
Manages piece discovery, loading, and coordination
"""

import os
import json
from plugin_interface import PluginInterface


class Plugin(PluginInterface):
    def __init__(self, game_engine):
        super().__init__(game_engine)
        self.pieces = {}
        self.piece_directory = "games/default/pieces/"
        
    def initialize(self):
        """Initialize by discovering and loading all pieces"""
        print("Piece Manager plugin initialized")
        self.discover_pieces()
        print(f"Loaded {len(self.pieces)} pieces")
    
    def discover_pieces(self):
        """Discover and load all pieces from the pieces directory"""
        if not os.path.exists(self.piece_directory):
            print(f"Piece directory {self.piece_directory} does not exist")
            return
        
        # Discover all piece directories
        for piece_name in os.listdir(self.piece_directory):
            piece_path = os.path.join(self.piece_directory, piece_name)
            if os.path.isdir(piece_path):
                # Look for the piece file (should be named the same as the directory)
                piece_file = os.path.join(piece_path, f"{piece_name}.txt")
                if os.path.exists(piece_file):
                    self.pieces[piece_name] = self.load_piece_from_file(piece_file)
                    print(f"Loaded piece: {piece_name}")
    
    def load_piece_from_file(self, file_path):
        """Load a piece's state from its text file"""
        piece_state = {}
        try:
            with open(file_path, 'r') as f:
                for line in f:
                    line = line.strip()
                    if line and not line.startswith('#'):
                        parts = line.split(' ', 1)
                        if len(parts) >= 2:
                            key = parts[0]
                            value = parts[1]
                            
                            # Try to convert numbers where appropriate
                            if key in ['option_count', 'current_selection', 'refresh_count', 'last_refresh_time', 'current_time', 'turn', 'keys_recorded', 'last_key_code', 'last_pressed_key_code']:
                                try:
                                    value = int(value)
                                except ValueError:
                                    pass  # Keep as string if not a valid int
                            elif key.startswith('option_'):
                                # Options are strings
                                pass
                            elif isinstance(value, str) and value.lower() in ('true', 'false'):
                                value = value.lower() == 'true'
                            elif key == 'last_pressed_key_char' and len(value) == 3 and value.startswith("'") and value.endswith("'"):
                                # Handle character literals like 'A'
                                value = value[1]  # Extract the character from 'A'
                            
                            piece_state[key] = value
                        elif len(parts) == 1:
                            # Handle single keywords
                            piece_state[parts[0]] = True
        except Exception as e:
            print(f"Error loading piece from {file_path}: {e}")
        
        return piece_state
    
    def get_piece_state(self, piece_name):
        """Get the current state of a piece"""
        return self.pieces.get(piece_name, None)
    
    def update_piece_state(self, piece_name, new_state):
        """Update the state of a piece and save it to file"""
        self.pieces[piece_name] = new_state

        # Save to file
        piece_path = os.path.join(self.piece_directory, piece_name, f"{piece_name}.txt")
        try:
            with open(piece_path, 'w') as f:
                for key, value in new_state.items():
                    # Convert certain values back to string format for saving
                    if key in ['option_count', 'current_selection', 'refresh_count', 'last_refresh_time', 'current_time', 'turn', 'keys_recorded', 'last_key_code', 'last_pressed_key_code']:
                        f.write(f"{key} {str(value)}\n")
                    else:
                        f.write(f"{key} {value}\n")
        except Exception as e:
            print(f"Error saving piece {piece_name}: {e}")
    
    def get_all_pieces(self):
        """Return all pieces"""
        return self.pieces