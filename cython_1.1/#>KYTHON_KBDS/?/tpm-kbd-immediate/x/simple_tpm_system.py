#!/usr/bin/env python3
"""
Simplified TPM Menu System - Demonstrating Core Concepts
Removes complex keyboard listener and display modification.

This version focuses on the core TPM principles:
- Each menu is a piece with its own state
- Input is captured and echoed immediately
- Navigation between menus/submenus as pieces
"""

import os
import sys
import time
from pathlib import Path
import select
import tty
import termios


class SimpleTPMSystem:
    """
    Simplified True Piece Method system that demonstrates core concepts
    without complex keyboard listeners and display modifications
    """
    
    def __init__(self):
        self.running = True
        self.current_menu = "main"
        self.setup_terminal()
        
        # Create pieces directory
        self.pieces_dir = Path("games/default/pieces")
        self.pieces_dir.mkdir(parents=True, exist_ok=True)
        
        # Initialize basic pieces
        self.init_pieces()
    
    def setup_terminal(self):
        """Setup terminal for raw input"""
        try:
            self.old_settings = termios.tcgetattr(sys.stdin)
            tty.setcbreak(sys.stdin.fileno())
        except:
            print("Note: Running in non-terminal mode")
    
    def restore_terminal(self):
        """Restore terminal to normal mode"""
        try:
            termios.tcsetattr(sys.stdin, termios.TCSADRAIN, self.old_settings)
        except:
            pass
    
    def init_pieces(self):
        """Initialize basic pieces for the menu system"""
        # Create main menu piece
        main_menu_file = self.pieces_dir / "main_menu" / "main_menu.txt"
        main_menu_file.parent.mkdir(exist_ok=True)
        
        if not main_menu_file.exists():
            with open(main_menu_file, 'w') as f:
                f.write("""title Main Menu
option_0 Play Game
option_1 Settings
option_2 Utilities
option_3 Help
option_4 Exit
option_count 5
current_selection 0
methods [show_menu, handle_selection]
""")
        
        # Create settings menu piece
        settings_menu_file = self.pieces_dir / "settings_menu" / "settings_menu.txt"
        settings_menu_file.parent.mkdir(exist_ok=True)
        
        if not settings_menu_file.exists():
            with open(settings_menu_file, 'w') as f:
                f.write("""title Settings
option_0 Audio
option_1 Video
option_2 Controls
option_3 Back
option_count 4
current_selection 0
methods [show_menu, handle_selection]
""")
    
    def get_input_immediate_echo(self):
        """Get input with immediate echo, similar to what was requested"""
        try:
            if select.select([sys.stdin], [], [], 0) == ([sys.stdin], [], []):
                ch = sys.stdin.read(1)
                
                # Echo the character immediately - this is what we want
                print(ch, end='', flush=True)
                
                # Handle special cases
                if ch == '\n' or ch == '\r':
                    print()  # Add newline for readability
                    return '\n'
                elif ch == '\x03':  # Ctrl+C
                    return 'CTRL_C'
                elif ch == '\x7f':  # Backspace
                    print('\b \b', end='', flush=True)
                    return 'BACKSPACE'
                elif ch == '\x1b':  # ESC sequence
                    # Try to read the rest of an arrow key sequence
                    try:
                        if select.select([sys.stdin], [], [], 0.01) == ([sys.stdin], [], []):
                            ch2 = sys.stdin.read(1)
                            if ch2 == '[':
                                ch3 = sys.stdin.read(1)
                                if ch3 == 'A':
                                    print("[UP]", end='', flush=True)
                                    return 'ARROW_UP'
                                elif ch3 == 'B':
                                    print("[DOWN]", end='', flush=True)
                                    return 'ARROW_DOWN'
                                elif ch3 == 'C':
                                    print("[RIGHT]", end='', flush=True)
                                    return 'ARROW_RIGHT'
                                elif ch3 == 'D':
                                    print("[LEFT]", end='', flush=True)
                                    return 'ARROW_LEFT'
                    except:
                        pass
                    return 'ESC'
                
                return ch
        except:
            # Handle different platforms that might not support select
            pass
        
        return None
    
    def process_input(self):
        """Process input in a simplified way"""
        ch = self.get_input_immediate_echo()
        
        if ch is None:
            return  # No input available
        
        if ch == 'CTRL_C':
            self.running = False
            print("\nExiting...")
        elif ch == '\n':
            # Handle complete command
            self.handle_complete_command()
        elif ch == 'ARROW_UP':
            print("\nUP ARROW pressed")
        elif ch == 'ARROW_DOWN':
            print("\nDOWN ARROW pressed")
        elif ch == 'ARROW_LEFT':
            print("\nLEFT ARROW pressed")
            # Navigate back in menu
            if self.current_menu != "main":
                self.current_menu = "main"
                self.display_menu()
        elif ch == 'ARROW_RIGHT':
            print("\nRIGHT ARROW pressed")
    
    def handle_complete_command(self):
        """Handle a complete command entered by user"""
        # For simplicity, we'll just demonstrate what would happen
        print("\nEnter a number (0-4) to navigate menus:")
        print("0-4: Select menu option")
        print("h: Show current menu")
        print("q: Quit")
        
    def display_menu(self):
        """Display the current menu by reading from its piece file"""
        print("\n" + "="*50)
        
        # Determine which menu file to read
        menu_filename = f"{self.current_menu}_menu.txt"
        menu_dir_name = f"{self.current_menu}_menu"
        menu_dir = self.pieces_dir / menu_dir_name
        menu_path = menu_dir / menu_filename
        
        if menu_dir.exists():
            menu_file = menu_dir / f"{self.current_menu}_menu.txt"
            if menu_file.exists():
                # Read and display the menu from its piece file
                print(f"TPM - Reading from piece: {menu_file}")
                print("Contents:")
                with open(menu_file, 'r') as f:
                    for line in f:
                        line = line.strip()
                        if line and not line.startswith('#'):
                            # For display purposes, format the line nicely
                            if line.startswith('title '):
                                title = line[6:]
                                print(f"{title}")
                                print("-" * len(title))
                            elif line.startswith('option_'):
                                parts = line.split(' ', 1)
                                if len(parts) > 1:
                                    print(f"  {parts[0]}: {parts[1]}")
                            elif line.startswith('option_count'):
                                print(f"  Available options: {line.split(' ')[1]}")
                            else:
                                print(f"  {line}")
            else:
                print(f"Menu piece file doesn't exist: {menu_file}")
        else:
            # Create a simple display fallback
            if self.current_menu == "main":
                print("MAIN MENU")
                print("---------")
                print("  option_0: Play Game")
                print("  option_1: Settings") 
                print("  option_2: Utilities")
                print("  option_3: Help")
                print("  option_4: Exit")
                print("  option_count: 5")
            elif self.current_menu == "settings":
                print("SETTINGS MENU")
                print("-------------")
                print("  option_0: Audio")
                print("  option_1: Video")
                print("  option_2: Controls") 
                print("  option_3: Back")
                print("  option_count: 4")
        
        print("="*50)
        print("Press keys to see immediate echo and use arrow keys to navigate menus")
        print("Press 'q' then Enter to quit")
    
    def run(self):
        """Run the simplified TPM system"""
        print("SIMPLIFIED TPM MENU SYSTEM")
        print("==========================")
        print("This demonstrates core TPM concepts:")
        print("- Menus implemented as pieces in /games/default/pieces/")
        print("- Keyboard input echoed immediately when pressed")
        print("- No complex keyboard listeners or display managers")
        print("- Simple navigation between menu pieces")
        print()
        
        self.display_menu()
        
        # Main loop - focus on input/output without complex listeners
        while self.running:
            self.process_input()
            time.sleep(0.01)  # Small delay for CPU


if __name__ == "__main__":
    system = SimpleTPMSystem()
    system.run()