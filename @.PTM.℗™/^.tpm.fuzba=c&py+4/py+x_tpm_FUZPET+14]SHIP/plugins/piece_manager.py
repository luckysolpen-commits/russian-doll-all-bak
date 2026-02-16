#!/usr/bin/env python3
"""
Module for managing file-based inter-process communication (IPC) between pieces.
This centralizes file operations for piece state management and communication.
"""

import os
import sys
import time


class PieceManager:
    """Manages file-based communication between different pieces."""
    
    def __init__(self):
        """Initialize the PieceManager."""
        self.script_dir = os.path.dirname(__file__)
    
    def get_path_for_piece(self, piece_name, filename):
        """Generate the appropriate file path for a given piece."""
        return os.path.join(self.script_dir, f'../pieces/{piece_name}/{filename}')
    
    def read_piece_state(self, piece_name):
        """Read state from a piece's state file (piece.txt)."""
        state_path = self.get_path_for_piece(piece_name, 'piece.txt')
        
        if not os.path.exists(state_path):
            print(f"Warning: State file not found for piece {piece_name} at {state_path}", file=sys.stderr)
            return {}
        
        state_data = {}
        try:
            with open(state_path, 'r') as f:
                lines = f.readlines()
                
            # Parse the state from the file
            for line in lines:
                line = line.strip()
                if '=' in line:
                    key, value = line.split('=', 1)
                    key = key.strip()
                    value = value.strip()
                    
                    # Convert string values back to appropriate types
                    if key in ['pos_x', 'pos_y', 'level']:
                        state_data[key] = int(value)
                    elif key in ['hunger', 'happiness', 'energy']:
                        state_data[key] = int(value)
                    elif key == 'name':
                        state_data[key] = value
                    else:
                        state_data[key] = value  # Default to string
        except Exception as e:
            print(f"Error reading piece state for {piece_name}: {e}", file=sys.stderr)
            
        return state_data
    
    def write_piece_state(self, piece_name, state_data):
        """Write state to a piece's state file (piece.txt)."""
        state_path = self.get_path_for_piece(piece_name, 'piece.txt')
        
        # Ensure directory exists
        os.makedirs(os.path.dirname(state_path), exist_ok=True)
        
        try:
            with open(state_path, 'w') as f:
                for key, value in state_data.items():
                    f.write(f"{key}={value}\n")
            return True
        except Exception as e:
            print(f"Error writing piece state for {piece_name}: {e}", file=sys.stderr)
            return False
    
    def load_map(self):
        """Load the world map from file."""
        map_path = self.get_path_for_piece('world', 'map.txt')
        
        if not os.path.exists(map_path):
            print("Map file not found, creating a default one.", file=sys.stderr)
            self.create_default_map()
        
        map_grid = []
        try:
            with open(map_path, 'r') as f:
                map_grid = [list(line.strip()) for line in f.readlines()]
        except Exception as e:
            print(f"Error loading map: {e}", file=sys.stderr)
        
        return map_grid
    
    def save_map(self, map_grid):
        """Save the world map to file."""
        map_path = self.get_path_for_piece('world', 'map.txt')
        
        # Ensure directory exists
        os.makedirs(os.path.dirname(map_path), exist_ok=True)
        
        try:
            with open(map_path, 'w') as f:
                for row in map_grid:
                    f.write("".join(row) + "\n")
            return True
        except Exception as e:
            print(f"Error saving map: {e}", file=sys.stderr)
            return False
    
    def create_default_map(self):
        """Create a default map if none exists."""
        map_path = self.get_path_for_piece('world', 'map.txt')
        
        # Ensure directory exists
        os.makedirs(os.path.dirname(map_path), exist_ok=True)
        
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
        
        with open(map_path, 'w') as f:
            f.write("\n".join(default_map))
    
    def save_frame(self, content):
        """Save the display frame to file."""
        frame_path = self.get_path_for_piece('display', 'current_frame.txt')
        tmp_path = self.get_path_for_piece('display', 'current_frame.txt.tmp')
        
        # Ensure directory exists
        os.makedirs(os.path.dirname(frame_path), exist_ok=True)
        
        try:
            # Write to a temporary file first
            with open(tmp_path, 'w') as f:
                f.write(content)
            
            # Atomically rename the temporary file to the final file
            os.rename(tmp_path, frame_path)
            return True
        except Exception as e:
            print(f"Error saving frame: {e}", file=sys.stderr)
            return False
    
    def read_keyboard_history(self):
        """Read the keyboard history file."""
        history_path = self.get_path_for_piece('keyboard', 'history.txt')
        
        if not os.path.exists(history_path):
            return []
        
        try:
            with open(history_path, 'r') as f:
                lines = f.readlines()
            return [line.strip() for line in lines]
        except Exception as e:
            print(f"Error reading keyboard history: {e}", file=sys.stderr)
            return []
    
    def append_to_master_ledger(self, event_type, description, source='piece_manager'):
        """Append an entry to the master ledger."""
        ledger_path = self.get_path_for_piece('master_ledger', 'ledger.txt')
        
        # Ensure directory exists
        os.makedirs(os.path.dirname(ledger_path), exist_ok=True)
        
        timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
        log_entry = f"[{timestamp}] {event_type}: {description} | Source: {source}\n"
        
        try:
            with open(ledger_path, 'a') as ledger:
                ledger.write(log_entry)
            return True
        except Exception as e:
            print(f"Error writing to master ledger: {e}", file=sys.stderr)
            return False


# Global instance for easy access if needed
piece_manager = PieceManager()


def main():
    """Main function for the piece manager - typically runs as part of other modules."""
    print("Piece Manager initialized.", file=sys.stderr)
    # The piece manager usually operates as a utility module imported by other pieces


if __name__ == "__main__":
    main()