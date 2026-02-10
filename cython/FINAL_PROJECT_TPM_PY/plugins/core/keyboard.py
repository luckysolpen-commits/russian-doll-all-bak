"""
Keyboard plugin - Same as game's keyboard plugin but for menu system
Handles keyboard input logging to history file following kilorb.c standards
Works with keyboard piece to maintain input logs
"""

import os
import time
from plugin_interface import PluginInterface


class Plugin(PluginInterface):
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
        for plugin in self.game_engine.plugin_manager.plugin_instances:
            if hasattr(plugin, 'get_piece_state'):
                self.piece_manager = plugin
                break
        
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
            # Create directory if it doesn't exist
            os.makedirs(os.path.dirname(self.history_file_path), exist_ok=True)
            
            # Open the history file in append mode (create if doesn't exist)
            with open(self.history_file_path, 'a') as f:
                # Write the key to the history file in the format similar to kilorb.c
                # For string keys, we'll log them in a readable format plus character code
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
            
            # Trigger keyboard event through the event system - similar to how dog responds to turn_end
            self._trigger_key_press_event(key)
            
            return True
        except Exception as e:
            print(f"Error writing key to history file: {e}")
            return False
    
    def _trigger_key_press_event(self, original_key):
        """Trigger a key press event that other plugins can listen for"""
        # Find the event manager plugin to trigger events
        event_plugin = None
        for plugin in self.game_engine.plugin_manager.plugin_instances:
            if hasattr(plugin, '_trigger_event_by_type') and hasattr(plugin, 'check_events'):
                event_plugin = plugin
                break
        
        if event_plugin:
            # Call the event manager to trigger responses for plugins listening to 'key_press'
            try:
                # Update the keyboard piece to include the specific key pressed
                keyboard_state = self.piece_manager.get_piece_state(self.keyboard_piece_name)
                if keyboard_state:
                    # Store the key that was pressed so listeners can access it
                    if isinstance(original_key, str) and len(original_key) == 1:
                        keyboard_state['last_pressed_key_code'] = ord(original_key)
                        keyboard_state['last_pressed_key_char'] = original_key
                    elif isinstance(original_key, int):
                        keyboard_state['last_pressed_key_code'] = original_key
                        keyboard_state['last_pressed_key_char'] = chr(original_key) if 32 <= original_key <= 126 else f"KEY_{original_key}"
                    else:
                        keyboard_state['last_pressed_key_code'] = 0
                        keyboard_state['last_pressed_key_char'] = str(original_key)
                    
                    self.piece_manager.update_piece_state(self.keyboard_piece_name, keyboard_state)
                
                # Trigger the 'key_press' event which will notify all plugins listening
                event_plugin._trigger_event_by_type('key_press')
            except Exception as e:
                print(f"Error triggering key press event: {e}")
    
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