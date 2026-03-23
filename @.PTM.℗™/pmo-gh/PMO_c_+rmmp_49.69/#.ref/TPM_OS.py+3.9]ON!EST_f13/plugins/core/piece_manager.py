"""
Piece Manager Plugin for PyOS-TPM
Manages loading, storing, and accessing piece data following TPM principles
"""

import os
import json
from pathlib import Path
from plugin_interface import PluginInterface


class Plugin(PluginInterface):
    def __init__(self, engine):
        super().__init__(engine)
        self.pieces_dir = Path("pieces")
        self.pieces = {}  # {piece_id: piece_data}
        self.piece_paths = {}  # {piece_id: directory_path}
        self.load_all_pieces()
    
    def load_all_pieces(self):
        """Load all pieces from the pieces directory"""
        for piece_dir_name in os.listdir(self.pieces_dir):
            piece_path = self.pieces_dir / piece_dir_name
            if piece_path.is_dir():
                piece_txt_path = piece_path / "piece.txt"
                if piece_txt_path.exists():
                    self.piece_paths[piece_dir_name] = piece_path
                    self.pieces[piece_dir_name] = self.parse_piece(piece_txt_path)
    
    def parse_piece(self, piece_txt_path):
        """Parse a piece.txt file and return its data as a dictionary"""
        piece_data = {
            'state': {},
            'methods': [],
            'event_listeners': [],
            'responses': {},
            'relations': {}
        }
        
        with open(piece_txt_path, 'r') as f:
            lines = f.readlines()
        
        current_section = None
        for line in lines:
            line = line.strip()
            
            # Skip comments and empty lines
            if line.startswith('#') or not line:
                continue
            
            # Identify section headers
            if line.endswith(':') and ':' in line[:-1]:  # e.g., "state:", "methods:"
                section = line[:-1].strip()  # Remove trailing colon
                if section in piece_data:
                    current_section = section
                continue
            
            # Parse content based on current section
            if current_section == 'state':
                if ':' in line and not line.startswith('  '):  # Top-level key-value
                    continue  # Skip section markers
                elif line.startswith('  ') and ':' in line:  # Indented key-value pair
                    parts = line.split(':', 1)
                    if len(parts) == 2:
                        key = parts[0].strip().strip(' -')  # Remove dash from list items
                        value_str = parts[1].strip()
                        # Try to parse as boolean, list, or string
                        if value_str.lower() in ('true', 'false'):
                            piece_data['state'][key] = value_str.lower() == 'true'
                        elif value_str.startswith('[') and value_str.endswith(']'):
                            # Parse list: ["item1", "item2"] or item1, item2
                            list_content = value_str[1:-1]  # Remove brackets
                            if ',' in list_content:
                                piece_data['state'][key] = [item.strip().strip('"\'') for item in list_content.split(',')]
                            else:
                                piece_data['state'][key] = [item.strip().strip('"\'') for item in list_content.split()]
                        else:
                            # Try to parse as integer or float, fall back to string
                            try:
                                if '.' in value_str:
                                    piece_data['state'][key] = float(value_str)
                                else:
                                    piece_data['state'][key] = int(value_str)
                            except ValueError:
                                piece_data['state'][key] = value_str
            elif current_section in ['methods', 'event_listeners']:
                if line.startswith('  ') and not ':' in line[:line.find(line.lstrip()[0]) if line.lstrip() else 0]:
                    # This is a method/listener in a list format
                    value = line.strip().strip(' -')
                    if value:
                        # If it's the first item, it might be a comma-separated list
                        if current_section == 'methods' and ',' in value:
                            methods = [m.strip() for m in value.split(',')]
                            piece_data[current_section].extend(methods)
                        elif current_section == 'event_listeners' and ',' in value:
                            listeners = [l.strip() for l in value.split(',')]
                            piece_data[current_section].extend(listeners)
                        else:
                            piece_data[current_section].append(value)
            elif current_section == 'responses':
                if ':' in line and line.startswith('  '):
                    parts = line.split(':', 1)
                    if len(parts) == 2:
                        key = parts[0].strip().strip(' -')
                        value = parts[1].strip()
                        piece_data['responses'][key] = value
            elif current_section == 'relations':
                if ':' in line and line.startswith('  '):
                    parts = line.split(':', 1)
                    if len(parts) == 2:
                        key = parts[0].strip().strip(' -')
                        value_str = parts[1].strip()
                        if value_str.startswith('[') and value_str.endswith(']'):
                            # Parse list
                            list_content = value_str[1:-1]
                            if ',' in list_content:
                                piece_data['relations'][key] = [item.strip().strip('"\'') for item in list_content.split(',')]
                            else:
                                piece_data['relations'][key] = [item.strip().strip('"\'') for item in list_content.split()]
                        else:
                            # Handle simple values
                            if value_str.lower() in ('true', 'false'):
                                piece_data['relations'][key] = value_str.lower() == 'true'
                            elif value_str.isdigit():
                                piece_data['relations'][key] = int(value_str)
                            else:
                                piece_data['relations'][key] = value_str
    
        return piece_data
    
    def get_piece_state(self, piece_id, key=None):
        """Get the state of a piece, optionally a specific key"""
        if piece_id not in self.pieces:
            return None
        
        state = self.pieces[piece_id].get('state', {})
        if key:
            return state.get(key)
        return state
    
    def update_piece_state(self, piece_id, new_values):
        """Update the state of a piece and save to file"""
        if piece_id not in self.pieces:
            return False
        
        # Update the in-memory state
        current_state = self.pieces[piece_id].get('state', {})
        current_state.update(new_values)
        self.pieces[piece_id]['state'] = current_state
        
        # Write the updated state back to the piece file
        self._write_piece_state_to_file(piece_id)
        
        # Log the state change to the master ledger for audit trail
        import time
        timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
        for key, value in new_values.items():
            log_entry = f"[{timestamp}] StateChange: {piece_id} {key} {value} | Source: piece_manager\n"
            with open('pieces/master_ledger/ledger.txt', 'a') as ledger:
                ledger.write(log_entry)
        
        return True
    
    def _write_piece_state_to_file(self, piece_id):
        """Write the updated state back to the piece.txt file"""
        if piece_id not in self.pieces or piece_id not in self.piece_paths:
            return
        
        piece_path = self.piece_paths[piece_id] / "piece.txt"
        piece_data = self.pieces[piece_id]
        
        # Read the current file to preserve non-state sections
        with open(piece_path, 'r') as f:
            lines = f.readlines()
        
        # Write the file back with updated state
        with open(piece_path, 'w') as f:
            in_state_section = False
            state_written = False
            
            for line in lines:
                if line.strip() == 'state:':
                    f.write(line)
                    in_state_section = True
                    state_written = True
                    # Write all state values
                    for key, value in piece_data['state'].items():
                        if isinstance(value, bool):
                            f.write(f"  {key}: {str(value).lower()}\n")
                        elif isinstance(value, list):
                            # Format list as comma-separated values in brackets
                            formatted_list = ', '.join([f'"{item}"' if isinstance(item, str) else str(item) for item in value])
                            f.write(f"  {key}: [{formatted_list}]\n")
                        elif isinstance(value, (int, float)):
                            f.write(f"  {key}: {value}\n")
                        else:
                            f.write(f"  {key}: {value}\n")
                elif in_state_section and line.startswith('  ') and not line.startswith('    ') and ':' in line:
                    # Skip old state values since we're rewriting them
                    continue
                elif in_state_section and not line.startswith('  '):
                    # End of state section
                    in_state_section = False
                    f.write(line)
                else:
                    if not state_written or not in_state_section:
                        f.write(line)
    
    def initialize(self):
        """Initialize the piece manager"""
        print(f"Piece Manager loaded {len(self.pieces)} pieces")
    
    def handle_input(self, key):
        """Handle input - not typically used for piece manager"""
        pass
    
    def update(self, dt):
        """Update piece manager - typically just monitor changes"""
        # In a real implementation, we might monitor for file changes
        pass