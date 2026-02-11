#!/usr/bin/env python3
"""
Basic Menu System demonstrating True Piece Method (TPM) architecture
This system removes keyboard listener method execution and display modification,
instead using a generic menu system that echoes keyboard input immediately
when pressed, with menus and submenus implemented as pieces.
"""

import os
import sys
import time
import tty
import termios
import select
from pathlib import Path


class BasicMenuSystem:
    """
    A basic menu system that demonstrates TPM concepts:
    - Echoes keyboard input immediately when pressed
    - Shows menus and submenus that are also pieces
    - Demonstrates how any project can be built with TPM
    """
    
    def __init__(self):
        self.running = True
        self.current_menu = "main"
        self.input_buffer = []
        self.setup_terminal()
        
        # Initialize the basic piece structure
        self.init_pieces()
        
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
    
    def init_pieces(self):
        """Initialize piece structure for menus"""
        # Create base pieces directory if it doesn't exist
        self.pieces_dir = Path("games/default/pieces")
        self.pieces_dir.mkdir(parents=True, exist_ok=True)
        
        # Create menu directories for each menu piece
        menu_pieces = ["main_menu", "settings_menu", "game_menu", "help_menu"]
        
        for menu_name in menu_pieces:
            menu_dir = self.pieces_dir / menu_name
            menu_dir.mkdir(exist_ok=True)
            
            # Create the main piece file
            piece_file = menu_dir / f"{menu_name}.txt"
            if not piece_file.exists():
                self.create_menu_piece(menu_name)
    
    def create_menu_piece(self, menu_name):
        """Create a piece file for a specific menu"""
        menu_dir = self.pieces_dir / menu_name
        piece_file = menu_dir / f"{menu_name}.txt"
        
        # Define menu structure based on menu name
        if menu_name == "main_menu":
            content = f"""title Main Menu
option_0 Play Game
option_1 Settings
option_2 Help
option_3 Exit
option_count 4
current_selection 0
event_listeners [key_press]
methods [show_menu, handle_selection, navigate_menu]
"""
        elif menu_name == "settings_menu":
            content = f"""title Settings Menu
option_0 Sound
option_1 Graphics
option_2 Controls
option_3 Back
option_count 4
current_selection 0
event_listeners [key_press]
methods [show_menu, handle_selection, navigate_menu]
"""
        elif menu_name == "game_menu":
            content = f"""title Game Menu
option_0 Start New Game
option_1 Load Game
option_2 Save Game
option_3 Main Menu
option_count 4
current_selection 0
event_listeners [key_press]
methods [show_menu, handle_selection, navigate_menu]
"""
        elif menu_name == "help_menu":
            content = f"""title Help Menu
option_0 Instructions
option_1 About
option_2 Credits
option_3 Back
option_count 4
current_selection 0
event_listeners [key_press]
methods [show_menu, handle_selection, navigate_menu]
"""
        else:
            content = f"""title Generic Menu
option_0 Option 1
option_1 Option 2
option_2 Option 3
option_3 Back
option_count 4
current_selection 0
event_listeners [key_press]
methods [show_menu, handle_selection, navigate_menu]
"""
        
        with open(piece_file, 'w') as f:
            f.write(content)
    
    def load_menu_piece(self, menu_name):
        """Load a menu piece from its text file"""
        menu_dir = self.pieces_dir / menu_name
        piece_file = menu_dir / f"{menu_name}.txt"
        
        if not piece_file.exists():
            self.create_menu_piece(menu_name)
        
        # Parse the menu piece file
        menu_data = {}
        with open(piece_file, 'r') as f:
            for line in f:
                line = line.strip()
                if line and not line.startswith('#'):
                    parts = line.split(' ', 1)
                    if len(parts) >= 2:
                        key = parts[0]
                        value = parts[1]
                        menu_data[key] = value
        
        return menu_data
    
    def get_available_input(self):
        """Get any available keyboard input without blocking"""
        try:
            if select.select([sys.stdin], [], [], 0) == ([sys.stdin], [], []):
                ch = sys.stdin.read(1)
                
                # Echo the character immediately
                print(ch, end='', flush=True)
                
                # Handle special cases like newlines
                if ch == '\n' or ch == '\r':
                    print()  # Print newline
                
                return ch
        except:
            # In case select is not available (e.g., Windows or non-terminal)
            # Use a non-blocking read alternative
            pass
        
        return None
    
    def process_keyboard_input(self):
        """Poll for keyboard input and process it immediately"""
        # Get any available input
        ch = self.get_available_input()
        
        if ch is not None:
            # Add to input buffer for processing
            self.input_buffer.append(ch)
            
            # If it's Enter, process the accumulated input
            if ch == '\n' or ch == '\r':
                # Process the whole line
                input_line = ''.join(self.input_buffer[:-1])  # Exclude the newline
                self.handle_input(input_line)
                self.input_buffer = []  # Clear the buffer
            elif ch == '\x03':  # Ctrl+C
                self.running = False
                print("^C")
            elif ch == '\x7f' or ch == '\x08':  # Backspace
                if self.input_buffer:
                    self.input_buffer.pop()  # Remove last character
                    print('\b \b', end='', flush=True)  # Erase from screen
            elif ch == '\x1b':  # Escape sequence
                # Try to read the rest of the escape sequence
                try:
                    if select.select([sys.stdin], [], [], 0.01) == ([sys.stdin], [], []):
                        ch2 = sys.stdin.read(1)
                        if ch2 == '[':
                            ch3 = sys.stdin.read(1)
                            if ch3 in ['A', 'B', 'C', 'D']:  # Arrow keys
                                # Echo arrow key indication
                                arrow_names = {'A': '↑', 'B': '↓', 'C': '→', 'D': '←'}
                                print(f"[{arrow_names[ch3]}]", end='', flush=True)
                                
                                # Process arrow key
                                if ch3 == 'A':  # Up
                                    self.handle_input('UP')
                                elif ch3 == 'B':  # Down
                                    self.handle_input('DOWN')
                                elif ch3 == 'C':  # Right
                                    self.handle_input('RIGHT')
                                elif ch3 == 'D':  # Left
                                    self.handle_input('LEFT')
                except:
                    pass
    
    def handle_input(self, input_str):
        """Handle the processed input string"""
        input_lower = input_str.lower().strip()
        
        # Echo the complete input for debugging
        print(f"\nReceived input: '{input_str}'")
        
        # Process menu navigation based on current menu
        if self.current_menu == "main":
            if input_lower in ['1', 'up', 'k']:
                self.show_menu("main_menu")
            elif input_lower in ['2', 'down', 'j']:
                self.show_menu("settings_menu")
            elif input_lower in ['3', 'right', 'l']:
                self.show_menu("game_menu")
            elif input_lower in ['4', 'left', 'h']:
                self.show_menu("help_menu")
            elif input_lower in ['q', 'quit', 'exit']:
                self.running = False
            else:
                print(f"Unknown command: {input_str}")
                print("Use 1-4 to navigate menus, or 'q' to quit")
        
        elif self.current_menu.startswith("main_menu"):
            if input_lower in ['0', 'play', 'start', '1']:
                print("Starting new game...")
                time.sleep(1)
                self.show_menu("game_menu")
            elif input_lower in ['1', 'settings', 'config', '2']:
                self.show_menu("settings_menu")
            elif input_lower in ['2', 'help', 'info', '3']:
                self.show_menu("help_menu")
            elif input_lower in ['3', 'exit', 'quit', '4']:
                self.running = False
            elif input_lower in ['back', 'b', 'main', 'menu']:
                self.show_menu("main")
            else:
                print(f"Option not recognized: {input_str}")
        
        elif self.current_menu.startswith("settings_menu"):
            if input_lower in ['0', 'sound', 'audio', '1']:
                print("Sound settings...")
                time.sleep(1)
                self.show_menu("settings_menu")
            elif input_lower in ['1', 'graphics', 'video', '2']:
                print("Graphics settings...")
                time.sleep(1)
                self.show_menu("settings_menu")
            elif input_lower in ['2', 'controls', 'input', '3']:
                print("Controls settings...")
                time.sleep(1)
                self.show_menu("settings_menu")
            elif input_lower in ['3', 'back', 'b', '4']:
                self.show_menu("main_menu")
            elif input_lower in ['main', 'menu']:
                self.show_menu("main")
            else:
                print(f"Setting not recognized: {input_str}")
        
        elif self.current_menu.startswith("game_menu"):
            if input_lower in ['0', 'new', 'start', '1']:
                print("Starting new game...")
                time.sleep(1)
                print("New game started!")
            elif input_lower in ['1', 'load', 'open', '2']:
                print("Loading game...")
                time.sleep(1)
                print("Game loaded!")
            elif input_lower in ['2', 'save', 'store', '3']:
                print("Saving game...")
                time.sleep(1)
                print("Game saved!")
            elif input_lower in ['3', 'back', 'b', '4']:
                self.show_menu("main_menu")
            elif input_lower in ['main', 'menu']:
                self.show_menu("main")
            else:
                print(f"Game option not recognized: {input_str}")
        
        elif self.current_menu.startswith("help_menu"):
            if input_lower in ['0', 'instructions', 'guide', '1']:
                print("Showing instructions...")
                time.sleep(1)
                print("Use arrow keys or number keys to navigate menus.")
                print("Press Enter to select options.")
                print("Press 'q' to quit.")
            elif input_lower in ['1', 'about', 'version', '2']:
                print("About this system...")
                time.sleep(1)
                print("TPM Basic Menu System - Demonstrates True Piece Method architecture")
            elif input_lower in ['2', 'credits', 'authors', '3']:
                print("Credits...")
                time.sleep(1)
                print("Developed using TPM (True Piece Method) principles")
            elif input_lower in ['3', 'back', 'b', '4']:
                self.show_menu("main_menu")
            elif input_lower in ['main', 'menu']:
                self.show_menu("main")
            else:
                print(f"Help option not recognized: {input_str}")
    
    def show_menu(self, menu_name):
        """Display a menu by loading its piece data"""
        print("\n" + "="*50)
        
        if menu_name == "main":
            print("TPM BASIC MENU SYSTEM - DEMONSTRATION")
            print("="*50)
            print("Welcome to the TPM demonstration system!")
            print("This shows how any project can be built with TPM.")
            print()
            print("Press number keys 1-4 to navigate to different menus:")
            print("1 - Main Menu    (↑)  (use arrow keys too)")
            print("2 - Settings     (↓)")
            print("3 - Game Options (→)")
            print("4 - Help         (←)")
            print("q - Quit")
            print()
            print("Notice that your keyboard input is echoed immediately!")
            print("Try typing and see how each character appears instantly.")
            self.current_menu = menu_name
        else:
            # Load the menu from its piece file
            menu_data = self.load_menu_piece(menu_name)
            title = menu_data.get('title', menu_name.replace('_', ' ').title())
            option_count = int(menu_data.get('option_count', 0))
            
            print(f"TPM - {title}")
            print("="*50)
            
            # Print each option
            for i in range(option_count):
                option = menu_data.get(f'option_{i}', f'Option {i}')
                print(f"{i} - {option}")
            
            print()
            print("Navigate with number keys, arrow keys, or type option names.")
            print("Press 'back' or 'menu' to return to main menu.")
            self.current_menu = menu_name
    
    def run(self):
        """Main run loop"""
        print("Starting TPM Basic Menu System...")
        print("Press Ctrl+C to exit at any time")
        print()
        
        # Show initial menu
        self.show_menu("main")
        
        # Main loop
        while self.running:
            self.process_keyboard_input()
            time.sleep(0.01)  # Small delay to prevent excessive CPU usage
        
        print("\nExiting TPM Basic Menu System...")
        self.restore_terminal()


if __name__ == "__main__":
    system = BasicMenuSystem()
    system.run()