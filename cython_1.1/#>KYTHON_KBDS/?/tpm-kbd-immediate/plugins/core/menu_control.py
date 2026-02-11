#!/usr/bin/env python3
"""
Menu Control Plugin - Adapted from RPG Control Plugin
Handles keyboard input for menu navigation (arrows, numbers) and translates to menu events
Works with menu piece to process navigation and selections
"""

import tty
import termios
import select
from plugin_interface import PluginInterface


class Plugin(PluginInterface):
    def __init__(self, game_engine):
        super().__init__(game_engine)
        self.piece_manager = None
        self.control_piece_name = 'menu_control'
        self.display_plugin = None
        self.keyboard_plugin = None  # Reference to keyboard plugin to poll for inputs
        self.last_known_key_code = None  # Track the last processed key
        self.current_menu = 'main_menu'  # Track current menu
        
    def initialize(self):
        """Initialize menu control plugin by connecting to required pieces"""
        print("Menu Control plugin initialized")
        
        # Find the piece manager plugin
        for plugin in self.game_engine.plugin_manager.plugin_instances:
            if hasattr(plugin, 'get_piece_state'):
                self.piece_manager = plugin
                break
        
        # Find display and keyboard plugins
        for plugin in self.game_engine.plugin_manager.plugin_instances:
            if hasattr(plugin, 'refresh_display'):
                self.display_plugin = plugin
            # Find keyboard plugin specifically
            elif hasattr(plugin, 'get_new_inputs') and hasattr(plugin, 'log_key'):
                self.keyboard_plugin = plugin
        
        # Initialize the menu control piece
        if self.piece_manager:
            # Create menu control state if it doesn't exist
            control_state = self.piece_manager.get_piece_state(self.control_piece_name)
            if not control_state:
                control_state = {
                    'current_menu': 'main_menu',
                    'current_selection': 0,
                    'initialized': True,
                    'supported_inputs': ['NUMBERS', 'ARROWS', 'NAVIGATION'],
                    'last_action': 'initialized'
                }
                self.piece_manager.update_piece_state(self.control_piece_name, control_state)
            else:
                control_state['initialized'] = True
                control_state['supported_inputs'] = ['NUMBERS', 'ARROWS', 'NAVIGATION']
                self.piece_manager.update_piece_state(self.control_piece_name, control_state)
    
    def handle_input(self, key):
        """Receive input but don't process it directly - it will be read from the history file"""
        # The keyboard plugin will handle the logging of this input
        # This method exists so the plugin system can still call it if needed,
        # but the actual processing is done by the update method polling the history file
        return False  # Don't consume the key, let other systems handle it
    
    def _handle_number_input(self, num_char):
        """Handle number key input for menu selection"""
        if not self.piece_manager:
            return False
            
        try:
            option_num = int(num_char)  # Convert character to number
            # Get current menu piece
            menu_state = self.piece_manager.get_piece_state(self.current_menu)
            if not menu_state:
                return False
                
            option_count = int(menu_state.get('option_count', 0))
            if 0 <= option_num < option_count:
                # Update the current selection
                menu_state['current_selection'] = option_num
                self.piece_manager.update_piece_state(self.current_menu, menu_state)
                
                # Execute the menu option
                self._execute_menu_option(option_num)
                
                # Update control piece
                control_state = self.piece_manager.get_piece_state(self.control_piece_name)
                if control_state:
                    control_state['current_selection'] = option_num
                    control_state['last_action'] = f'Selected option {option_num}'
                    self.piece_manager.update_piece_state(self.control_piece_name, control_state)
                
                return True
        except ValueError:
            pass  # Not a valid number
        
        return False
    
    def _handle_navigation_input(self, key_code):
        """Handle navigation key input for menu navigation"""
        if not self.piece_manager:
            return False
        
        menu_state = self.piece_manager.get_piece_state(self.current_menu)
        if not menu_state:
            return False
        
        option_count = int(menu_state.get('option_count', 0))
        current_selection = int(menu_state.get('current_selection', 0))
        
        if key_code == 1000:  # UP ARROW
            new_selection = max(0, current_selection - 1)
        elif key_code == 1001:  # DOWN ARROW
            new_selection = min(option_count - 1, current_selection + 1)
        elif key_code == 1002:  # RIGHT ARROW or Enter-like action
            # Execute the currently selected option
            self._execute_menu_option(current_selection)
            return True
        elif key_code == 1003:  # LEFT ARROW - go back to parent menu
            parent_menu = menu_state.get('parent_menu', 'main_menu')
            if parent_menu and parent_menu != 'none':
                self.current_menu = parent_menu
                # Update menu control state
                control_state = self.piece_manager.get_piece_state(self.control_piece_name)
                if control_state:
                    control_state['current_menu'] = self.current_menu
                    control_state['current_selection'] = 0
                    control_state['last_action'] = f'Navigated to {self.current_menu}'
                    self.piece_manager.update_piece_state(self.control_piece_name, control_state)
                # Reset parent menu selection
                parent_state = self.piece_manager.get_piece_state(self.current_menu)
                if parent_state:
                    parent_state['current_selection'] = 0
                    self.piece_manager.update_piece_state(self.current_menu, parent_state)
            return True
        else:
            return False
        
        # Update selection
        menu_state['current_selection'] = new_selection
        self.piece_manager.update_piece_state(self.current_menu, menu_state)
        
        # Update control piece
        control_state = self.piece_manager.get_piece_state(self.control_piece_name)
        if control_state:
            control_state['current_selection'] = new_selection
            control_state['last_action'] = f'Moved selection to {new_selection}'
            self.piece_manager.update_piece_state(self.control_piece_name, control_state)
        
        return True
    
    def _execute_menu_option(self, option_num):
        """Execute a menu option based on its number"""
        menu_state = self.piece_manager.get_piece_state(self.current_menu)
        if not menu_state:
            return
        
        option_key = f'option_{option_num}'
        option_text = menu_state.get(option_key, f'Option {option_num}')
        
        print(f"\nExecuting: {option_text}")  # Immediate feedback
        
        # Handle specific menu options
        if self.current_menu == 'main_menu':
            if option_num == 0:  # Play Game
                print("Opening game...")
            elif option_num == 1:  # Settings
                print("Opening settings...")
            elif option_num == 2:  # Utilities
                print("Opening utilities...")
            elif option_num == 3:  # Help
                print("Opening help...")
        elif self.current_menu == 'submenu':
            if option_num == 3:  # Back to Main
                self.current_menu = 'main_menu'
            else:
                print(f"SubMenu option {option_num} selected")
        elif self.current_menu == 'settings_menu':
            if option_num == 3:  # Back
                parent_menu = menu_state.get('parent_menu', 'main_menu')
                self.current_menu = parent_menu
        elif self.current_menu == 'utilities_menu':
            if option_num == 4:  # Back
                parent_menu = menu_state.get('parent_menu', 'main_menu')
                self.current_menu = parent_menu
        
        # Update menu control piece
        control_state = self.piece_manager.get_piece_state(self.control_piece_name)
        if control_state:
            control_state['current_menu'] = self.current_menu
            control_state['last_action'] = f'Activated option {option_num}'
            self.piece_manager.update_piece_state(self.control_piece_name, control_state)
            
        # Trigger display refresh if available
        if self.display_plugin:
            self.display_plugin.refresh_display()
    
    def _trigger_quit_event(self):
        """Set flag to quit the menu system"""
        if self.piece_manager:
            control_state = self.piece_manager.get_piece_state(self.control_piece_name)
            if control_state:
                control_state['quit_requested'] = True
                control_state['last_action'] = 'Quit requested'
                self.piece_manager.update_piece_state(self.control_piece_name, control_state)
    
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
        # Map key codes to menu actions
        if ord('0') <= key_code <= ord('9'):  # Number keys 0-9
            # Convert ASCII code back to character and handle
            num_char = chr(key_code)
            self._handle_number_input(num_char)
        elif key_code in [1000, 1001, 1002, 1003]:  # Arrow keys
            self._handle_navigation_input(key_code)
        else:
            # Handle other key codes if needed
            pass
    
    def render(self, screen):
        """Render control info to screen if needed"""
        pass