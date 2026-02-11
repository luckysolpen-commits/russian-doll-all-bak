"""
Control plugin for ASCII Roguelike game
Handles keyboard input (arrows, WASD) and translates to game events
Works with control piece to process movement and other inputs
"""

import tty
import termios
import select
from plugin_interface import PluginInterface


class Plugin(PluginInterface):
    def __init__(self, game_engine):
        super().__init__(game_engine)
        self.piece_manager = None
        self.control_piece_name = 'keyboard'  # Using keyboard piece for control
        self.player_plugin = None
        self.world_plugin = None
        self.keyboard_plugin = None  # Reference to keyboard plugin to poll for inputs
        self.last_known_key_code = None  # Track the last processed key
        
    def initialize(self):
        """Initialize control plugin by connecting to required pieces"""
        print("Control plugin initialized")
        
        # Find the piece manager plugin
        for plugin in self.game_engine.plugin_manager.plugin_instances:
            if hasattr(plugin, 'get_piece_state'):
                self.piece_manager = plugin
                break
        
        # Find player and world plugins
        for plugin in self.game_engine.plugin_manager.plugin_instances:
            if hasattr(plugin, 'get_player_position'):
                self.player_plugin = plugin
            elif hasattr(plugin, 'get_tile'):
                self.world_plugin = plugin
            # Find keyboard plugin specifically
            elif hasattr(plugin, 'get_new_inputs') and hasattr(plugin, 'log_key'):
                self.keyboard_plugin = plugin
        
        # Update control piece state to reflect initialization
        if self.piece_manager:
            control_state = self.piece_manager.get_piece_state(self.control_piece_name)
            if control_state:
                control_state['initialized'] = True
                control_state['supported_inputs'] = ['WASD', 'ARROWS', 'MENU_NUMBERS']
                self.piece_manager.update_piece_state(self.control_piece_name, control_state)
    
    def handle_input(self, key):
        """Receive input but don't process it directly - it will be read from the history file"""
        # The keyboard plugin will handle the logging of this input
        # This method exists so the plugin system can still call it if needed,
        # but the actual processing is done by the update method polling the history file
        return False  # Don't consume the key, let other systems handle it
    
    def _handle_directional_input(self, key):
        """Handle WASD input"""
        # Map keys to directional vectors
        key_map = {
            'w': (0, -1),  # Up
            's': (0, 1),   # Down
            'a': (-1, 0),  # Left
            'd': (1, 0)    # Right
        }
        
        if key in key_map and self.player_plugin:
            dx, dy = key_map[key]
            # Call the player's move method which should update the piece manager
            if hasattr(self.player_plugin, 'move_player'):
                return self.player_plugin.move_player(dx, dy)
        
        return False
    
    def _handle_arrow_input(self, key):
        """Handle arrow key input"""
        # Arrow keys typically send \x1b[A, \x1b[B, \x1b[C, \x1b[D
        if len(key) >= 3:
            if key.endswith('[A'):  # Up arrow
                dx, dy = 0, -1
            elif key.endswith('[B'):  # Down arrow
                dx, dy = 0, 1
            elif key.endswith('[C'):  # Right arrow
                dx, dy = 1, 0
            elif key.endswith('[D'):  # Left arrow
                dx, dy = -1, 0
            else:
                return False
        else:
            # Handle standard format
            if key == '\x1b[A':  # Up arrow
                dx, dy = 0, -1
            elif key == '\x1b[B':  # Down arrow
                dx, dy = 0, 1
            elif key == '\x1b[C':  # Right arrow
                dx, dy = 1, 0
            elif key == '\x1b[D':  # Left arrow
                dx, dy = -1, 0
            else:
                return False
        
        if self.player_plugin:
            return self.player_plugin.move_player(dx, dy)
        
        return False
    
    def _handle_number_input(self, num_char):
        """Handle number key input for menu selection"""
        if not self.piece_manager:
            return False
            
        try:
            option_num = int(num_char)  # Convert character to number
            # Trigger menu event to handle option
            # Update menu piece to indicate current selection
            menu_state = self.piece_manager.get_piece_state('menu')
            if menu_state:
                menu_state['current_selection'] = option_num
                self.piece_manager.update_piece_state('menu', menu_state)
                
                # Also store as last menu choice for display plugin to handle
                control_state = self.piece_manager.get_piece_state(self.control_piece_name)
                if control_state:
                    control_state['last_menu_choice'] = option_num
                    self.piece_manager.update_piece_state(self.control_piece_name, control_state)
                
                return True
        except ValueError:
            pass  # Not a valid number
        
        return False
    
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
            for plugin in self.game_engine.plugin_manager.plugin_instances:
                if hasattr(plugin, 'get_new_inputs') and hasattr(plugin, 'log_key'):
                    self.keyboard_plugin = plugin
                    break
            if not self.keyboard_plugin:
                return []
        
        # Use the keyboard plugin's method to get new inputs since it has the file tracking
        return self.keyboard_plugin.get_new_inputs()
    
    def _process_key_event(self, key_code):
        """Process a specific key code as an event based on kilorb.c enum values"""
        # Map key codes to actions based on kilorb.c enum
        if ord('0') <= key_code <= ord('9'):  # Number keys 0-9
            num_char = chr(key_code)
            self._handle_number_input(num_char)
        elif key_code == ord('w') or key_code == ord('W'):  # W key
            self._handle_directional_input('w')
        elif key_code == ord('s') or key_code == ord('S'):  # S key
            self._handle_directional_input('s')
        elif key_code == ord('a') or key_code == ord('A'):  # A key
            self._handle_directional_input('a')
        elif key_code == ord('d') or key_code == ord('D'):  # D key
            self._handle_directional_input('d')
        elif key_code == 1000:  # ARROW_UP
            self._handle_arrow_input('\x1b[A')
        elif key_code == 1001:  # ARROW_DOWN
            self._handle_arrow_input('\x1b[B')
        elif key_code == 1002:  # ARROW_RIGHT
            self._handle_arrow_input('\x1b[C')
        elif key_code == 1003:  # ARROW_LEFT
            self._handle_arrow_input('\x1b[D')
        else:
            # Handle other key codes if needed
            pass
    
    def render(self, screen):
        """Render control info to screen if needed"""
        pass