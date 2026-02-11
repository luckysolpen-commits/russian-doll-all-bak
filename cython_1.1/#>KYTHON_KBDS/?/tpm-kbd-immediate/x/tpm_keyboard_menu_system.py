#!/usr/bin/env python3
"""
TPM Keyboard-First Menu System - Maintains Full TPM Compliance
This system follows the same pipeline as the control plugin but for menu interactions.
"""

import sys
import os
import tty
import termios
import select
import time
from pathlib import Path


class PluginInterface:
    """Base interface that all plugins must implement"""
    def __init__(self, game_engine):
        self.game_engine = game_engine
        self.enabled = True
    
    def initialize(self):
        """Called when the plugin is loaded"""
        pass
    
    def activate(self):
        """Called when the plugin is activated"""
        self.enabled = True
    
    def deactivate(self):
        """Called when the plugin is deactivated""" 
        self.enabled = False
    
    def handle_input(self, key):
        """Handle input events"""
        pass
    
    def update(self, dt):
        """Update plugin state"""
        pass
    
    def render(self, screen):
        """Render plugin content to screen"""
        pass


class PluginManager:
    """Manages loading, initializing, and coordinating plugins"""
    
    def __init__(self):
        self.plugin_instances = []  # Store plugin instances
        self.loaded_plugins = {}    # Track by name

    def register_plugin(self, plugin_name, plugin_instance):
        """Register a new plugin instance"""
        self.plugin_instances.append(plugin_instance)
        self.loaded_plugins[plugin_name] = plugin_instance
        print(f"Plugin registered: {plugin_name}")
    
    def initialize_all_plugins(self):
        """Call initialize method on all loaded plugins"""
        for plugin in self.plugin_instances:
            try:
                plugin.initialize()
            except AttributeError:
                # Plugin doesn't have initialize method, which is fine
                pass
    
    def handle_input_for_all_plugins(self, key):
        """Forward input to all plugins"""
        for plugin in self.plugin_instances:
            try:
                plugin.handle_input(key)
            except AttributeError:
                # Plugin doesn't have handle_input method, which is fine
                pass
    
    def update_all_plugins(self, dt):
        """Update all plugins"""
        for plugin in self.plugin_instances:
            try:
                plugin.update(dt)
            except AttributeError:
                # Plugin doesn't have update method, which is fine
                pass

    def find_plugin_by_capability(self, capability_attr):
        """Find a plugin based on a specific capability"""
        for plugin in self.plugin_instances:
            if hasattr(plugin, capability_attr):
                return plugin
        return None


class PieceManagerPlugin(PluginInterface):
    """Manages piece discovery, loading, and coordination - TPM compliant"""
    
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
            os.makedirs(self.piece_directory, exist_ok=True)
            print(f"Created piece directory: {self.piece_directory}")
            # Create a default menu piece
            self.create_default_pieces()
        
        # Discover all piece directories
        for piece_name in os.listdir(self.piece_directory):
            piece_path = os.path.join(self.piece_directory, piece_name)
            if os.path.isdir(piece_path):
                # Look for the piece file (should be named the same as the directory)
                piece_file = os.path.join(piece_path, f"{piece_name}.txt")
                if os.path.exists(piece_file):
                    self.pieces[piece_name] = self.load_piece_from_file(piece_file)
                    print(f"Loaded piece: {piece_name}")
        
        # Also create default menu pieces if they don't exist
        self.create_default_pieces()
        
    def create_default_pieces(self):
        """Create default menu pieces for the system"""
        # Create menu directory structure
        menu_dir = os.path.join(self.piece_directory, "main_menu")
        os.makedirs(menu_dir, exist_ok=True)
        
        # Create main menu piece file
        main_menu_file = os.path.join(menu_dir, "main_menu.txt")
        if not os.path.exists(main_menu_file):
            with open(main_menu_file, 'w') as f:
                f.write("""title Main Menu
option_0 Open Submenu
option_1 Settings
option_2 Utilities 
option_3 Exit
option_count 4
current_selection 0
event_listeners [key_press, menu_select]
methods [show_menu, handle_selection, navigate_menu]
status active
initialized true
""")
        
        # Create submenu piece
        submenu_dir = os.path.join(self.piece_directory, "submenu")
        os.makedirs(submenu_dir, exist_ok=True)
        
        submenu_file = os.path.join(submenu_dir, "submenu.txt")
        if not os.path.exists(submenu_file):
            with open(submenu_file, 'w') as f:
                f.write("""title Sub Menu
option_0 Option 1
option_1 Option 2
option_2 Option 3
option_3 Back to Main
option_count 4
current_selection 0
event_listeners [key_press, menu_select]
methods [show_menu, handle_selection, navigate_menu]
status active
parent_menu main_menu
""")
        
        # Create keyboard piece
        keyboard_dir = os.path.join(self.piece_directory, "keyboard")
        os.makedirs(keyboard_dir, exist_ok=True)
        
        keyboard_file = os.path.join(keyboard_dir, "keyboard.txt")
        if not os.path.exists(keyboard_file):
            with open(keyboard_file, 'w') as f:
                f.write("""session_id 12345
current_key_log_path games/default/pieces/keyboard/history.txt
logging_enabled true
keys_recorded 0
event_listeners [key_press]
last_key_logged 
last_key_time 
last_key_code 0
methods [log_key, reset_session, get_session_log, enable_logging, disable_logging]
status active
""")
        
        # Reload pieces after creation
        self.discover_pieces()
        
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
                            if key == 'option_count' or key == 'current_selection':
                                try:
                                    value = int(value)
                                except ValueError:
                                    pass  # Keep as string if not a valid int
                            elif key.startswith('option_'):
                                # Options are strings
                                pass
                            elif value.lower() in ('true', 'false'):
                                value = value.lower() == 'true'
                            
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
                    f.write(f"{key} {value}\n")
        except Exception as e:
            print(f"Error saving piece {piece_name}: {e}")
    
    def get_all_pieces(self):
        """Return all pieces"""
        return self.pieces


class KeyboardPlugin(PluginInterface):
    """Handles keyboard input logging to history file - maintains the same pipeline as control plugin"""
    
    def __init__(self, game_engine):
        super().__init__(game_engine)
        self.piece_manager = None
        self.keyboard_piece_name = 'keyboard'
        self.history_file_path = 'games/default/pieces/keyboard/history.txt'
        self.session_started = False
        self.last_processed_position = 0  # Track where other plugins left off reading
        
    def initialize(self):
        """Initialize keyboard plugin by connecting to keyboard piece"""
        print("Keyboard plugin initialized")
        
        # Find the piece manager plugin
        self.piece_manager = self.game_engine.plugin_manager.find_plugin_by_capability('get_piece_state')
        
        # Initialize the session - clear history file and update piece state
        if self.piece_manager:
            self.reset_session()
            # Update keyboard piece state
            keyboard_state = self.piece_manager.get_piece_state(self.keyboard_piece_name)
            if keyboard_state:
                keyboard_state['session_id'] = int(time.time())  # Use timestamp as session ID
                keyboard_state['keys_recorded'] = 0
                keyboard_state['last_session_start'] = time.strftime('%Y-%m-%d %H:%M:%S')
                self.piece_manager.update_piece_state(self.keyboard_piece_name, keyboard_state)
    
    def log_key(self, key):
        """Log a key press to the history file"""
        if not self.piece_manager:
            return False
            
        # Get current keyboard state
        keyboard_state = self.piece_manager.get_piece_state(self.keyboard_piece_name)
        if not keyboard_state or keyboard_state.get('logging_enabled', False) == False:
            return False
        
        try:
            # Open the history file in append mode (create if doesn't exist)
            os.makedirs(os.path.dirname(self.history_file_path), exist_ok=True)
            with open(self.history_file_path, 'a') as f:
                # Write the key to the history file in the format similar to kilorb.c
                if isinstance(key, str):
                    if len(key) == 1:
                        # Single character - write just the ASCII code like kilorb.c
                        f.write(f"{ord(key)}\n")  # Format: just the key code like in kilorb.c writeCommand
                    elif key.startswith('\x1b'):  # Escape sequence (arrow keys and others)
                        # For escape sequences, we need to parse them like in kilorb.c editorReadKey
                        if len(key) >= 3:
                            seq = key[1:]  # Get the part after \x1b
                            if len(seq) >= 2:
                                if seq[0] == '[':
                                    if seq[1] == 'A':  # Up arrow
                                        f.write(f"1000\n")  # ARROW_UP = 1000 in kilorb.c
                                    elif seq[1] == 'B':  # Down arrow
                                        f.write(f"1001\n")  # ARROW_DOWN = 1001 in kilorb.c
                                    elif seq[1] == 'C':  # Right arrow
                                        f.write(f"1002\n")  # ARROW_RIGHT = 1002 in kilorb.c
                                    elif seq[1] == 'D':  # Left arrow
                                        f.write(f"1003\n")  # ARROW_LEFT = 1003 in kilorb.c
                                    elif seq[1] >= '0' and seq[1] <= '9':  # Numbered sequences
                                        if len(seq) >= 3 and seq[2] == '~':
                                            if seq[1] == '1':
                                                f.write(f"1005\n")  # HOME_KEY in kilorb.c
                                            elif seq[1] == '3':
                                                f.write(f"1004\n")  # DEL_KEY in kilorb.c
                                            elif seq[1] == '4':
                                                f.write(f"1006\n")  # END_KEY in kilorb.c
                                            elif seq[1] == '5':
                                                f.write(f"1007\n")  # PAGE_UP in kilorb.c
                                            elif seq[1] == '6':
                                                f.write(f"1008\n")  # PAGE_DOWN in kilorb.c
                                            elif seq[1] == '7':
                                                f.write(f"1005\n")  # HOME_KEY in kilorb.c
                                            elif seq[1] == '8':
                                                f.write(f"1006\n")  # END_KEY in kilorb.c
                                        else:
                                            f.write(f"27\n")  # Unknown numbered sequence - just log ESC
                                    else:
                                        f.write(f"27\n")  # Other [ sequences - log ESC
                                elif seq[0] == 'O':  # O sequences
                                    if seq[1] == 'H':
                                        f.write(f"1005\n")  # HOME_KEY in kilorb.c
                                    elif seq[1] == 'F':
                                        f.write(f"1006\n")  # END_KEY in kilorb.c
                                    else:
                                        f.write(f"27\n")  # Other O sequences - log ESC
                                else:
                                    f.write(f"27\n")  # Other escape sequences - log ESC
                            else:
                                f.write(f"27\n")  # Short escape sequence - log ESC
                        else:
                            f.write(f"27\n")  # Very short escape sequence - log ESC
                    else:
                        # Multi-character string - just log the first character's code
                        if key:
                            f.write(f"{ord(key[0])}\n")
                        else:
                            f.write(f"0\n")
                else:
                    # Numeric key - log as-is followed by a readable label if needed
                    key_label = self._get_key_label(key)
                    if key_label:
                        f.write(f"{key}:{key_label}\n")
                    else:
                        f.write(f"{key}:KEY\n")
        
            # Update keyboard piece state to reflect new count
            keyboard_state = self.piece_manager.get_piece_state(self.keyboard_piece_name)
            if keyboard_state:
                current_count = keyboard_state.get('keys_recorded', 0)
                keyboard_state['keys_recorded'] = current_count + 1
                keyboard_state['last_key_logged'] = key
                keyboard_state['last_key_time'] = time.strftime('%Y-%m-%d %H:%M:%S')
                # Also record the key that triggered the event for other plugins to access
                keyboard_state['last_key_code'] = ord(key) if isinstance(key, str) and len(key) == 1 else (key if isinstance(key, int) else 0)
                self.piece_manager.update_piece_state(self.keyboard_piece_name, keyboard_state)
            
            # Trigger keyboard event through the event system
            self._trigger_key_press_event(key)
            
            return True
        except Exception as e:
            print(f"Error writing key to history file: {e}")
            return False
    
    def _trigger_key_press_event(self, original_key):
        """Trigger a key press event that other plugins can listen for"""
        # Find the event manager plugin to trigger events (we'll create this next)
        # For now, we'll just make sure other plugins can respond to keyboard events
        pass
    
    def reset_session(self):
        """Reset the session by clearing the history file"""
        try:
            # Create directory if it doesn't exist
            os.makedirs(os.path.dirname(self.history_file_path), exist_ok=True)
            
            # Clear the history file (create empty or truncate existing)
            with open(self.history_file_path, 'w') as f:
                # Optionally add a session header
                f.write(f"# Keyboard session started at {time.strftime('%Y-%m-%d %H:%M:%S')}\n")
                f.write("# Format: key_code (numeric)\n")
            self.session_started = True
            self.last_processed_position = 0  # Reset tracking
            print(f"Keyboard history file reset: {self.history_file_path}")
        except Exception as e:
            print(f"Error resetting keyboard history file: {e}")

    def get_new_inputs(self):
        """Get new inputs from the history file that weren't processed yet."""
        try:
            with open(self.history_file_path, 'r') as f:
                f.seek(self.last_processed_position)
                new_content = f.read()
                current_position = f.tell()
                
            # Store the new position to track for next call
            self.last_processed_position = current_position
            
            # Parse the new inputs
            if new_content:
                lines = new_content.strip().split('\n')
                inputs = []
                for line in lines:
                    line = line.strip()
                    if line and not line.startswith('#'):  # Skip comments
                        try:
                            key_code = int(line.split(':')[0])  # Get the numeric key code
                            inputs.append(key_code)
                        except ValueError:
                            # Line might not be a valid key code, skip it
                            continue
                return inputs
            return []
        except Exception as e:
            print(f"Error reading new inputs from history file: {e}")
            return []

    def get_last_key_code(self):
        """Get the most recent key code from the file"""
        try:
            with open(self.history_file_path, 'r') as f:
                lines = f.readlines()
                
            # Go through lines backwards to find the last valid key code
            for line in reversed(lines):
                line = line.strip()
                if line and not line.startswith('#'):
                    try:
                        key_code = int(line.split(':')[0])  # Get the numeric key code
                        return key_code
                    except ValueError:
                        continue
            return None
        except Exception as e:
            print(f"Error getting last key from history file: {e}")
            return None
    
    def _get_key_label(self, key_code):
        """Get a human-readable label for special keys"""
        # Define mappings similar to the enum in kilorb.c
        key_labels = {
            8: 'BACKSPACE',
            127: 'BACKSPACE',
            1000: 'ARROW_UP',
            1001: 'ARROW_DOWN', 
            1002: 'ARROW_RIGHT',
            1003: 'ARROW_LEFT',
            1004: 'DEL_KEY',
            1005: 'HOME_KEY',
            1006: 'END_KEY',
            1007: 'PAGE_UP',
            1008: 'PAGE_DOWN',
        }
        return key_labels.get(key_code, None)
    
    def handle_input(self, key):
        """Handle input by logging it to the history file"""
        # Log the key press to the history file
        self.log_key(key)
        # Don't consume the key, allow other plugins to handle it too
        return False
    
    def enable_logging(self):
        """Enable keyboard logging"""
        if self.piece_manager:
            keyboard_state = self.piece_manager.get_piece_state(self.keyboard_piece_name)
            if keyboard_state:
                keyboard_state['logging_enabled'] = True
                self.piece_manager.update_piece_state(self.keyboard_piece_name, keyboard_state)
    
    def disable_logging(self):
        """Disable keyboard logging"""
        if self.piece_manager:
            keyboard_state = self.piece_manager.get_piece_state(self.keyboard_piece_name)
            if keyboard_state:
                keyboard_state['logging_enabled'] = False
                self.piece_manager.update_piece_state(self.keyboard_piece_name, keyboard_state)
    
    def get_session_log(self):
        """Get the current session's key log"""
        try:
            if os.path.exists(self.history_file_path):
                with open(self.history_file_path, 'r') as f:
                    return f.read()
            return ""
        except Exception as e:
            print(f"Error reading session log: {e}")
            return ""
    
    def update(self, dt):
        """Update keyboard plugin state"""
        pass
    
    def render(self, screen):
        """Render keyboard info if needed"""
        pass


class ControlPlugin(PluginInterface):
    """Handles keyboard input and translates to menu events - same pipeline as game control"""
    
    def __init__(self, game_engine):
        super().__init__(game_engine)
        self.piece_manager = None
        self.control_piece_name = 'main_menu'  # Use main menu as control piece
        self.keyboard_plugin = None  # Reference to keyboard plugin to poll for inputs
        self.last_known_key_code = None  # Track the last processed key
        self.current_menu = 'main_menu'
        
    def initialize(self):
        """Initialize control plugin by connecting to required pieces"""
        print("Control plugin initialized")
        
        # Find the piece manager plugin
        self.piece_manager = self.game_engine.plugin_manager.find_plugin_by_capability('get_piece_state')
        
        # Find keyboard plugin specifically
        self.keyboard_plugin = self.game_engine.plugin_manager.find_plugin_by_capability('get_new_inputs')
        
        # Update control piece state to reflect initialization
        if self.piece_manager:
            control_state = self.piece_manager.get_piece_state(self.control_piece_name)
            if control_state:
                control_state['initialized'] = True
                control_state['supported_inputs'] = ['NUMBERS', 'ARROWS', 'NAVIGATION']
                control_state['current_menu'] = self.current_menu
                self.piece_manager.update_piece_state(self.control_piece_name, control_state)
    
    def handle_input(self, key):
        """Receive input but don't process it directly - it will be read from the history file"""
        # The keyboard plugin will handle the logging of this input
        # This method exists so the plugin system can still call it if needed,
        # but the actual processing is done by the update method polling the history file
        return False  # Don't consume the key, let other systems handle it
    
    def _handle_number_input(self, num_key):
        """Handle number key input for menu selection"""
        try:
            option_num = int(chr(num_key))  # Convert ASCII to number
            if self.piece_manager:
                menu_state = self.piece_manager.get_piece_state(self.current_menu)
                if menu_state:
                    option_count = menu_state.get('option_count', 0)
                    if 0 <= option_num < option_count:
                        # Update the current selection
                        menu_state['current_selection'] = option_num
                        self.piece_manager.update_piece_state(self.current_menu, menu_state)
                        
                        # Execute the menu option
                        self._execute_menu_option(option_num)
                        return True
        except ValueError:
            pass
        
        return False
    
    def _handle_arrow_input(self, key_code):
        """Handle arrow key input for menu navigation"""
        if not self.piece_manager:
            return False
        
        menu_state = self.piece_manager.get_piece_state(self.current_menu)
        if not menu_state:
            return False
        
        option_count = menu_state.get('option_count', 0)
        current_selection = menu_state.get('current_selection', 0)
        
        if key_code == 1000:  # UP ARROW
            new_selection = max(0, current_selection - 1)
        elif key_code == 1001:  # DOWN ARROW
            new_selection = min(option_count - 1, current_selection + 1)
        elif key_code == 1002:  # RIGHT ARROW
            # Navigate deeper into menu (move to selected option if it's a submenu)
            self._execute_menu_option(current_selection)
            return True
        elif key_code == 1003:  # LEFT ARROW
            # Navigate back to parent menu
            parent_menu = menu_state.get('parent_menu', 'main_menu')
            if parent_menu and parent_menu != 'none':
                self.current_menu = parent_menu
                menu_state = self.piece_manager.get_piece_state(self.current_menu)
                if menu_state:
                    menu_state['current_selection'] = 0
                    self.piece_manager.update_piece_state(self.current_menu, menu_state)
            return True
        else:
            return False
        
        menu_state['current_selection'] = new_selection
        self.piece_manager.update_piece_state(self.current_menu, menu_state)
        return True
    
    def _execute_menu_option(self, option_num):
        """Execute a menu option based on its number"""
        menu_state = self.piece_manager.get_piece_state(self.current_menu)
        if not menu_state:
            return
        
        option_key = f'option_{option_num}'
        option_text = menu_state.get(option_key, f'Option {option_num}')
        
        print(f"\nExecuting: {option_text}")
        
        # Handle specific menu options
        if self.current_menu == 'main_menu':
            if option_num == 0:  # Open Submenu
                self.current_menu = 'submenu'
                submenu_state = self.piece_manager.get_piece_state('submenu')
                if submenu_state:
                    submenu_state['current_selection'] = 0
                    self.piece_manager.update_piece_state('submenu', submenu_state)
            elif option_num == 1:  # Settings
                print("Opening settings...")
            elif option_num == 2:  # Utilities
                print("Opening utilities...")
            elif option_num == 3:  # Exit
                self._trigger_exit_event()
        elif self.current_menu == 'submenu':
            if option_num == 3:  # Back to Main
                self.current_menu = 'main_menu'
                main_menu_state = self.piece_manager.get_piece_state('main_menu')
                if main_menu_state:
                    main_menu_state['current_selection'] = 0
                    self.piece_manager.update_piece_state('main_menu', main_menu_state)
            else:
                print(f"SubMenu option {option_num} selected")
        
        # Update the control piece with current menu
        control_state = self.piece_manager.get_piece_state('main_menu')  # Using main_menu as control
        if control_state:
            control_state['current_menu'] = self.current_menu
            control_state['last_action'] = f'Selected option {option_num}'
            self.piece_manager.update_piece_state('main_menu', control_state)
    
    def _trigger_exit_event(self):
        """Set flag to exit the application"""
        # Find the game engine or set a flag that other parts can check
        if self.piece_manager:
            exit_state = self.piece_manager.get_piece_state('keyboard')  # Use keyboard piece for exit indicator
            if exit_state:
                exit_state['quit_requested'] = True
                self.piece_manager.update_piece_state('keyboard', exit_state)
    
    def update(self, dt):
        """Update control by polling for new inputs from keyboard history file directly"""
        # Poll the keyboard history file directly to get new inputs
        new_inputs = self._poll_keyboard_history()
        
        for key_code in new_inputs:
            # Process the key code using event-driven approach
            self._process_key_event(key_code)
    
    def _poll_keyboard_history(self):
        """Poll the keyboard history file for new entries"""
        if not self.keyboard_plugin:
            # Find the keyboard plugin to get the history file path
            self.keyboard_plugin = self.game_engine.plugin_manager.find_plugin_by_capability('get_new_inputs')
            if not self.keyboard_plugin:
                return []
        
        # Use the keyboard plugin's method to get new inputs since it has the file tracking
        return self.keyboard_plugin.get_new_inputs()
    
    def _process_key_event(self, key_code):
        """Process a specific key code as an event based on kilorb.c enum values"""
        # Map key codes to actions based on number keys and arrows
        if ord('0') <= key_code <= ord('9'):  # Number keys 0-9
            self._handle_number_input(key_code)
        elif key_code in [1000, 1001, 1002, 1003]:  # Arrow keys
            self._handle_arrow_input(key_code)
        elif key_code == ord('q') or key_code == ord('Q'):  # Q key for quit
            self._trigger_exit_event()
        else:
            # Handle other key codes if needed
            pass
    
    def render(self, screen):
        """Render control info to screen if needed"""
        pass


class DisplayPlugin(PluginInterface):
    """Handles rendering of menu system - shows menus and submenus as pieces"""
    
    def __init__(self, game_engine):
        super().__init__(game_engine)
        self.piece_manager = None
        self.control_plugin = None
        
    def initialize(self):
        """Initialize display plugin by connecting to required pieces"""
        print("Display plugin initialized")
        
        # Find the piece manager plugin
        self.piece_manager = self.game_engine.plugin_manager.find_plugin_by_capability('get_piece_state')
        
        # Find the control plugin to get current menu
        self.control_plugin = self.game_engine.plugin_manager.find_plugin_by_capability('current_menu')
    
    def update(self, dt):
        """Update display based on changes in menu state"""
        if not self.piece_manager:
            return
        
        # Get current menu from control plugin or default to main
        current_menu = 'main_menu'
        if self.control_plugin and hasattr(self.control_plugin, 'current_menu'):
            current_menu = self.control_plugin.current_menu
        
        # Display the current menu
        self._display_menu(current_menu)
    
    def _display_menu(self, menu_name):
        """Display a specific menu by loading its piece data"""
        menu_state = self.piece_manager.get_piece_state(menu_name)
        if not menu_state:
            print(f"Menu {menu_name} not found!")
            return
        
        title = menu_state.get('title', menu_name.replace('_', ' ').title())
        option_count = menu_state.get('option_count', 0)
        current_selection = menu_state.get('current_selection', 0)
        
        # Clear screen and display menu
        print("\n" + "="*60)
        print(f"TPM MENU SYSTEM - {title}")
        print("="*60)
        
        # Display all options with selection indicator
        for i in range(option_count):
            option_text = menu_state.get(f'option_{i}', f'Option {i}')
            if i == current_selection:
                print(f"> {i} - {option_text} <- (selected)")
            else:
                print(f"  {i} - {option_text}")
        
        print()
        print("CONTROLS:")
        print("- Press number keys (0-9) to select options")
        print("- Use arrow keys to navigate (UP/DOWN to move, LEFT to go back, RIGHT to confirm)")
        print("- Press 'q' to quit")
        print("- Keyboard input is echoed immediately when pressed")
        print("="*60)
    
    def handle_input(self, key):
        """Handle display-specific input"""
        pass
    
    def render(self, screen):
        """Render display content"""
        pass


class TPMKeyboardMenuEngine:
    """Main engine that coordinates all plugins in TPM-compliant way"""
    
    def __init__(self):
        self.plugin_manager = PluginManager()
        self.running = True
        
        # Register all plugins
        self.piece_manager = PieceManagerPlugin(self)
        self.keyboard_plugin = KeyboardPlugin(self)
        self.control_plugin = ControlPlugin(self)
        self.display_plugin = DisplayPlugin(self)
        
        self.plugin_manager.register_plugin('core.piece_manager', self.piece_manager)
        self.plugin_manager.register_plugin('core.keyboard', self.keyboard_plugin) 
        self.plugin_manager.register_plugin('core.control', self.control_plugin)
        self.plugin_manager.register_plugin('ui.display', self.display_plugin)
        
        # Initialize all plugins
        self.plugin_manager.initialize_all_plugins()
    
    def run(self):
        """Run the main engine loop"""
        print("TPM KEYBOARD-FIRST MENU SYSTEM")
        print("==============================")
        print("This maintains full TPM compliance with the same pipeline as control plugin")
        print("but focused on keyboard-driven menu system without game-specific features")
        print()
        
        # Setup terminal for raw input
        old_settings = None
        try:
            old_settings = termios.tcgetattr(sys.stdin)
            tty.setcbreak(sys.stdin.fileno())
        except:
            print("Warning: Could not setup raw input mode")
        
        last_display_time = time.time()
        
        try:
            while self.running:
                # Check for exit condition
                keyboard_state = self.piece_manager.get_piece_state('keyboard')
                if keyboard_state and keyboard_state.get('quit_requested', False):
                    break
                
                # Handle any direct input (for immediate echo)
                try:
                    if select.select([sys.stdin], [], [], 0) == ([sys.stdin], [], []):
                        ch = sys.stdin.read(1)
                        # Echo immediately
                        print(ch, end='', flush=True)
                        # Pass to plugins for processing
                        self.plugin_manager.handle_input_for_all_plugins(ch)
                        
                        # If it's a carriage return, handle as command
                        if ch in ['\n', '\r']:
                            print()  # Add newline
                except:
                    pass
                
                # Update all plugins (this mirrors the control plugin update pattern)
                current_time = time.time()
                dt = current_time - last_display_time
                if dt > 0.1:  # Update display every 100ms
                    self.plugin_manager.update_all_plugins(0.1)
                    last_display_time = current_time
                else:
                    self.plugin_manager.update_all_plugins(0.01)  # More frequent updates for responsiveness
                
                time.sleep(0.01)  # Small delay to prevent excessive CPU usage
        
        finally:
            # Restore terminal
            if old_settings:
                try:
                    termios.tcsetattr(sys.stdin, termios.TCSADRAIN, old_settings)
                except:
                    pass
        
        print("\nTPM Keyboard Menu System exited.")


def main():
    engine = TPMKeyboardMenuEngine()
    engine.run()


if __name__ == "__main__":
    main()