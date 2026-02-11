#!/usr/bin/env python3
"""
Generic Menu System with Submenus as Pieces - Demonstrating TPM Architecture
This system demonstrates how menus and submenus are implemented as pieces in TPM.
"""

import os
import sys
import time
import tty
import termios
import select
from pathlib import Path
import json


class TPMPieceManager:
    """
    Manages pieces in the TPM architecture.
    Each menu and submenu is implemented as a piece with its own state.
    """
    
    def __init__(self):
        self.base_dir = Path("games/default/pieces")
        self.base_dir.mkdir(parents=True, exist_ok=True)
        self.pieces = {}
        
    def create_piece(self, piece_name, data):
        """Create a new piece with the given name and data"""
        piece_dir = self.base_dir / piece_name
        piece_dir.mkdir(exist_ok=True)
        
        piece_file = piece_dir / f"{piece_name}.txt"
        with open(piece_file, 'w') as f:
            for key, value in data.items():
                if isinstance(value, list):
                    f.write(f"{key} {' '.join(map(str, value))}\n")
                elif isinstance(value, dict):
                    f.write(f"{key} {json.dumps(value)}\n")
                else:
                    f.write(f"{key} {value}\n")
        
        self.pieces[piece_name] = data
        return data
    
    def load_piece(self, piece_name):
        """Load a piece from its text file"""
        piece_dir = self.base_dir / piece_name
        piece_file = piece_dir / f"{piece_name}.txt"
        
        if piece_file.exists():
            with open(piece_file, 'r') as f:
                data = {}
                for line in f:
                    line = line.strip()
                    if line and not line.startswith('#'):
                        parts = line.split(' ', 1)
                        if len(parts) >= 2:
                            key = parts[0]
                            value = parts[1]
                            
                            # Special handling for certain keys
                            if key.startswith('option_') or key in ['title', 'type', 'parent_menu']:
                                data[key] = value
                            elif key == 'option_count':
                                data[key] = int(value)
                            elif key == 'methods' or key == 'event_listeners':
                                # Parse list format: [item1, item2, ...]
                                if value.startswith('[') and value.endswith(']'):
                                    value = value[1:-1]  # Remove brackets
                                    data[key] = [item.strip().strip('"') for item in value.split(',')]
                                else:
                                    data[key] = value.split()
                            else:
                                data[key] = value
                self.pieces[piece_name] = data
                return data
        else:
            # Return a default piece if it doesn't exist
            default_data = {
                'title': piece_name.replace('_', ' ').title(),
                'type': 'menu',
                'option_count': 0,
                'current_selection': 0,
                'methods': ['show_menu', 'handle_selection'],
                'event_listeners': ['key_press']
            }
            return self.create_piece(piece_name, default_data)
    
    def update_piece_state(self, piece_name, new_data):
        """Update the state of a piece and save it to file"""
        # Update internal cache
        if piece_name in self.pieces:
            self.pieces[piece_name].update(new_data)
        else:
            self.pieces[piece_name] = new_data
        
        # Save to file
        piece_dir = self.base_dir / piece_name
        piece_dir.mkdir(exist_ok=True)
        
        piece_file = piece_dir / f"{piece_name}.txt"
        with open(piece_file, 'w') as f:
            for key, value in new_data.items():
                if isinstance(value, list):
                    f.write(f"{key} [{' '.join(map(str, value))}]\n")
                elif isinstance(value, dict):
                    f.write(f"{key} {json.dumps(value)}\n")
                else:
                    f.write(f"{key} {value}\n")
        
        return new_data


class GenericMenuSystem:
    """
    A generic menu system with submenus implemented as pieces.
    Demonstrates True Piece Method architecture where menus are pieces with methods and event listeners.
    """
    
    def __init__(self):
        self.running = True
        self.piece_manager = TPMPieceManager()
        self.current_menu = "main_menu"
        self.input_buffer = []
        self.setup_terminal()
        
        # Initialize the menu system pieces
        self.init_menu_pieces()
        
    def setup_terminal(self):
        """Setup terminal for raw input"""
        try:
            self.old_settings = termios.tcgetattr(sys.stdin)
            tty.setcbreak(sys.stdin.fileno())
        except:
            # Not in a terminal, use standard input instead
            print("Note: Running in non-terminal mode, some input features may be limited")
            pass
    
    def restore_terminal(self):
        """Restore terminal to normal mode"""
        try:
            termios.tcsetattr(sys.stdin, termios.TCSADRAIN, self.old_settings)
        except:
            pass
    
    def init_menu_pieces(self):
        """Initialize the menu pieces using TPM architecture"""
        # Create main menu piece
        main_menu_data = {
            'title': 'Main Menu',
            'type': 'menu',
            'option_0': 'Games',
            'option_1': 'Settings', 
            'option_2': 'Utilities',
            'option_3': 'Help',
            'option_4': 'Exit',
            'option_count': 5,
            'current_selection': 0,
            'parent_menu': 'none',
            'methods': ['show_menu', 'handle_selection', 'navigate_menu'],
            'event_listeners': ['key_press', 'menu_open']
        }
        self.piece_manager.create_piece("main_menu", main_menu_data)
        
        # Create games submenu
        games_menu_data = {
            'title': 'Games',
            'type': 'submenu',
            'option_0': 'New Game',
            'option_1': 'Load Game',
            'option_2': 'Save Game',
            'option_3': 'Options',
            'option_4': 'Back to Main',
            'option_count': 5,
            'current_selection': 0,
            'parent_menu': 'main_menu',
            'methods': ['show_menu', 'handle_selection', 'navigate_menu'],
            'event_listeners': ['key_press', 'menu_open']
        }
        self.piece_manager.create_piece("games_menu", games_menu_data)
        
        # Create settings submenu
        settings_menu_data = {
            'title': 'Settings',
            'type': 'submenu',
            'option_0': 'Audio',
            'option_1': 'Video', 
            'option_2': 'Controls',
            'option_3': 'Accessibility',
            'option_4': 'Back to Main',
            'option_count': 5,
            'current_selection': 0,
            'parent_menu': 'main_menu',
            'methods': ['show_menu', 'handle_selection', 'navigate_menu'],
            'event_listeners': ['key_press', 'menu_open']
        }
        self.piece_manager.create_piece("settings_menu", settings_menu_data)
        
        # Create utilities submenu
        utilities_menu_data = {
            'title': 'Utilities',
            'type': 'submenu',
            'option_0': 'File Manager',
            'option_1': 'Text Editor',
            'option_2': 'Calculator',
            'option_3': 'Clock',
            'option_4': 'Back to Main',
            'option_count': 5,
            'current_selection': 0,
            'parent_menu': 'main_menu',
            'methods': ['show_menu', 'handle_selection', 'navigate_menu'],
            'event_listeners': ['key_press', 'menu_open']
        }
        self.piece_manager.create_piece("utilities_menu", utilities_menu_data)
        
        # Create help submenu
        help_menu_data = {
            'title': 'Help',
            'type': 'submenu',
            'option_0': 'Instructions',
            'option_1': 'Documentation',
            'option_2': 'About',
            'option_3': 'Credits', 
            'option_4': 'Back to Main',
            'option_count': 5,
            'current_selection': 0,
            'parent_menu': 'main_menu',
            'methods': ['show_menu', 'handle_selection', 'navigate_menu'],
            'event_listeners': ['key_press', 'menu_open']
        }
        self.piece_manager.create_piece("help_menu", help_menu_data)
        
        # Create sub-submenu examples
        audio_menu_data = {
            'title': 'Audio Settings',
            'type': 'submenu',
            'option_0': 'Master Volume',
            'option_1': 'Music Volume',
            'option_2': 'SFX Volume', 
            'option_3': 'Voice Volume',
            'option_4': 'Back to Settings',
            'option_count': 5,
            'current_selection': 0,
            'parent_menu': 'settings_menu',
            'methods': ['show_menu', 'handle_selection', 'navigate_menu'],
            'event_listeners': ['key_press', 'menu_open']
        }
        self.piece_manager.create_piece("audio_menu", audio_menu_data)
        
        # Create file manager submenu
        file_manager_data = {
            'title': 'File Manager',
            'type': 'submenu',
            'option_0': 'View Files',
            'option_1': 'Create File',
            'option_2': 'Edit File',
            'option_3': 'Delete File',
            'option_4': 'Back to Utilities',
            'option_count': 5,
            'current_selection': 0,
            'parent_menu': 'utilities_menu',
            'methods': ['show_menu', 'handle_selection', 'navigate_menu'],
            'event_listeners': ['key_press', 'menu_open']
        }
        self.piece_manager.create_piece("file_manager", file_manager_data)
    
    def get_available_input(self):
        """Get any available keyboard input without blocking"""
        try:
            if select.select([sys.stdin], [], [], 0) == ([sys.stdin], [], []):
                ch = sys.stdin.read(1)
                
                # Echo the character immediately to demonstrate immediate response
                print(ch, end='', flush=True)
                
                # Special handling for newlines and control chars
                if ch == '\n' or ch == '\r':
                    print()  # Print newline for readability
                elif ch == '\x03':  # Ctrl+C
                    return '^C'
                elif ch == '\x7f' or ch == '\x08':  # Backspace
                    if self.input_buffer:
                        self.input_buffer.pop()
                        print('\b \b', end='', flush=True)
                elif ch == '\x1b':  # Escape sequence
                    try:
                        if select.select([sys.stdin], [], [], 0.01) == ([sys.stdin], [], []):
                            ch2 = sys.stdin.read(1)
                            if ch2 == '[':
                                ch3 = sys.stdin.read(1)
                                if ch3 in ['A', 'B', 'C', 'D']:  # Arrow keys
                                    arrow_names = {'A': '↑', 'B': '↓', 'C': '→', 'D': '←'}
                                    print(f"[{arrow_names[ch3]}]", end='', flush=True)
                                    return f"ARROW_{ch3}"
                    except:
                        pass
                
                return ch
        except:
            # Handle systems where select is not available
            pass
        
        return None
    
    def process_input(self):
        """Process any available input"""
        ch = self.get_available_input()
        
        if ch is not None:
            # Add to buffer except for special control chars
            if ch not in ['\n', '\r', '\x03'] and not ch.startswith('ARROW_'):
                self.input_buffer.append(ch)
            
            # Handle specific input cases
            if ch == '^C':
                self.running = False
            elif ch in ['\n', '\r']:
                # Process the accumulated input line
                input_line = ''.join(self.input_buffer)
                self.input_buffer = []
                self.handle_menu_input(input_line)
            elif ch.startswith('ARROW_'):
                # Process arrow key input
                if ch == 'ARROW_A':  # Up
                    self.handle_menu_input('UP')
                elif ch == 'ARROW_B':  # Down
                    self.handle_menu_input('DOWN')
                elif ch == 'ARROW_C':  # Right
                    self.handle_menu_input('RIGHT')
                elif ch == 'ARROW_D':  # Left
                    self.handle_menu_input('LEFT')
    
    def handle_menu_input(self, input_str):
        """Handle menu navigation input"""
        print(f"\nInput received: '{input_str}'")  # Echo the input
        
        # Convert input to lowercase for comparison
        inp = input_str.lower().strip()
        
        # Load current menu piece
        current_menu_data = self.piece_manager.load_piece(self.current_menu)
        option_count = int(current_menu_data.get('option_count', 0))
        
        # Handle different types of input
        try:
            # Check if input is a number corresponding to an option
            option_num = int(inp)
            if 0 <= option_num < option_count:
                self.execute_menu_option(option_num)
            else:
                print(f"Invalid option. Select 0-{option_count-1}")
        except ValueError:
            # Input is not a number, handle as command
            if inp in ['up', 'k']:
                # Move selection up
                current_selection = int(current_menu_data.get('current_selection', 0))
                new_selection = max(0, current_selection - 1)
                current_menu_data['current_selection'] = new_selection
                self.piece_manager.update_piece_state(self.current_menu, current_menu_data)
                self.show_current_menu()
            elif inp in ['down', 'j']:
                # Move selection down
                current_selection = int(current_menu_data.get('current_selection', 0))
                new_selection = min(option_count - 1, current_selection + 1)
                current_menu_data['current_selection'] = new_selection
                self.piece_manager.update_piece_state(self.current_menu, current_menu_data)
                self.show_current_menu()
            elif inp in ['back', 'b', 'backspace']:
                # Go back to parent menu
                parent_menu = current_menu_data.get('parent_menu', 'main_menu')
                if parent_menu != 'none':
                    self.current_menu = parent_menu
                    self.show_current_menu()
                else:
                    print("No parent menu to go back to.")
            elif inp in ['main', 'home', 'menu']:
                # Go to main menu
                self.current_menu = 'main_menu'
                self.show_current_menu()
            elif inp in ['quit', 'exit', 'q']:
                self.running = False
            else:
                # Try to match input to option text
                matched = False
                for i in range(option_count):
                    option_text = current_menu_data.get(f'option_{i}', '').lower()
                    if inp in option_text.lower() or inp == str(i):
                        self.execute_menu_option(i)
                        matched = True
                        break
                
                if not matched:
                    print(f"Unknown command: '{inp}'. Use numbers 0-{option_count-1} or 'back'/'quit'")
    
    def execute_menu_option(self, option_num):
        """Execute a specific menu option"""
        current_menu_data = self.piece_manager.load_piece(self.current_menu)
        option_text = current_menu_data.get(f'option_{option_num}', f'Option {option_num}')
        
        print(f"Selected: {option_text}")
        
        # Handle different menu options
        if self.current_menu == "main_menu":
            if option_num == 0:  # Games
                self.current_menu = "games_menu"
                self.show_current_menu()
            elif option_num == 1:  # Settings
                self.current_menu = "settings_menu"
                self.show_current_menu()
            elif option_num == 2:  # Utilities
                self.current_menu = "utilities_menu"
                self.show_current_menu()
            elif option_num == 3:  # Help
                self.current_menu = "help_menu"
                self.show_current_menu()
            elif option_num == 4:  # Exit
                self.running = False
            else:
                print(f"Main menu option {option_num} not handled")
        
        elif self.current_menu == "games_menu":
            if option_num == 0:  # New Game
                print("Starting new game...")
                time.sleep(1)
            elif option_num == 1:  # Load Game
                print("Loading game...")
                time.sleep(1)
            elif option_num == 2:  # Save Game
                print("Saving game...")
                time.sleep(1)
            elif option_num == 3:  # Options
                print("Game options...")
                time.sleep(1)
            elif option_num == 4:  # Back to Main
                self.current_menu = "main_menu"
                self.show_current_menu()
            else:
                print(f"Games menu option {option_num} not handled")
        
        elif self.current_menu == "settings_menu":
            if option_num == 0:  # Audio
                self.current_menu = "audio_menu"
                self.show_current_menu()
            elif option_num == 1:  # Video
                print("Video settings...")
                time.sleep(1)
                self.show_current_menu()
            elif option_num == 2:  # Controls
                print("Controls settings...")
                time.sleep(1)
                self.show_current_menu()
            elif option_num == 3:  # Accessibility
                print("Accessibility settings...")
                time.sleep(1)
                self.show_current_menu()
            elif option_num == 4:  # Back to Main
                self.current_menu = "main_menu"
                self.show_current_menu()
            else:
                print(f"Settings menu option {option_num} not handled")
        
        elif self.current_menu == "utilities_menu":
            if option_num == 0:  # File Manager
                self.current_menu = "file_manager"
                self.show_current_menu()
            elif option_num == 1:  # Text Editor
                print("Opening text editor...")
                time.sleep(1)
            elif option_num == 2:  # Calculator
                print("Opening calculator...")
                time.sleep(1)
            elif option_num == 3:  # Clock
                print("Opening clock...")
                time.sleep(1)
            elif option_num == 4:  # Back to Main
                self.current_menu = "main_menu"
                self.show_current_menu()
            else:
                print(f"Utilities menu option {option_num} not handled")
        
        elif self.current_menu == "help_menu":
            if option_num == 0:  # Instructions
                print("Showing instructions...")
                time.sleep(2)
            elif option_num == 1:  # Documentation
                print("Opening documentation...")
                time.sleep(1)
            elif option_num == 2:  # About
                print("About this TPM system...")
                print("Demonstrates how any project can be built with TPM (True Piece Method)")
                print("Each menu is implemented as a piece with its own state and methods.")
                time.sleep(3)
            elif option_num == 3:  # Credits
                print("TPM Architecture developed with piece-centered design principles.")
                time.sleep(2)
            elif option_num == 4:  # Back to Main
                self.current_menu = "main_menu"
                self.show_current_menu()
            else:
                print(f"Help menu option {option_num} not handled")
        
        elif self.current_menu == "audio_menu":
            if option_num in [0, 1, 2, 3]:  # Volume settings
                print(f"Adjusting volume setting {option_num}...")
                time.sleep(1)
            elif option_num == 4:  # Back to Settings
                self.current_menu = "settings_menu"
                self.show_current_menu()
            else:
                print(f"Audio menu option {option_num} not handled")
        
        elif self.current_menu == "file_manager":
            if option_num == 0:  # View Files
                print("Listing files...")
                time.sleep(1)
            elif option_num == 1:  # Create File
                print("Creating new file...")
                time.sleep(1)
            elif option_num == 2:  # Edit File
                print("Opening file editor...")
                time.sleep(1)
            elif option_num == 3:  # Delete File
                print("Deleting file...")
                time.sleep(1)
            elif option_num == 4:  # Back to Utilities
                self.current_menu = "utilities_menu"
                self.show_current_menu()
            else:
                print(f"File manager option {option_num} not handled")
    
    def show_current_menu(self):
        """Display the current menu by loading its piece data"""
        print("\n" + "="*60)
        
        # Load menu data from its piece
        menu_data = self.piece_manager.load_piece(self.current_menu)
        title = menu_data.get('title', self.current_menu.replace('_', ' ').title())
        option_count = int(menu_data.get('option_count', 0))
        current_selection = int(menu_data.get('current_selection', 0))
        
        print(f"TPM - {title}")
        print("="*60)
        
        # Display all options with selection indicator
        for i in range(option_count):
            option_text = menu_data.get(f'option_{i}', f'Option {i}')
            if i == current_selection:
                print(f"> {i} - {option_text} <- (selected)")
            else:
                print(f"  {i} - {option_text}")
        
        print()
        print("Controls:")
        print("- Use number keys (0-9) to select options")
        print("- Use 'up/down' or 'j/k' to navigate")
        print("- Type 'back', 'menu', or option names to navigate")
        print("- Use 'quit' or 'q' to exit")
        print("- Notice: Your keyboard input echoes immediately!")
        print("="*60)
    
    def run(self):
        """Run the main menu system loop"""
        print("TPM GENERIC MENU SYSTEM WITH SUBMENUS AS PIECES")
        print("=================================================")
        print("This demonstrates True Piece Method (TPM) architecture where:")
        print("- Each menu is implemented as a piece")
        print("- Submenus are also pieces with their own state")
        print("- Input is echoed immediately when pressed")
        print("- All interactions are managed through piece states")
        print()
        
        # Show initial menu
        self.show_current_menu()
        
        # Main loop
        while self.running:
            self.process_input()
            time.sleep(0.01)  # Small delay to prevent excessive CPU usage
        
        print("\nExiting TPM Generic Menu System...")
        self.restore_terminal()


if __name__ == "__main__":
    system = GenericMenuSystem()
    system.run()