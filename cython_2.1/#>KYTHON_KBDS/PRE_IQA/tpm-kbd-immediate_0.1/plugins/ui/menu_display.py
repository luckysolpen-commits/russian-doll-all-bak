"""
Menu Display Plugin - Adapted from RPG Display Plugin
Handles rendering of menu interface (menu options, navigation status, etc.)
Works with display piece to visualize menu state
"""

from plugin_interface import PluginInterface


class Plugin(PluginInterface):
    def __init__(self, game_engine):
        super().__init__(game_engine)
        self.piece_manager = None
        self.control_plugin = None  # Reference to menu control plugin
        self.display_piece = None
        
    def initialize(self):
        """Initialize menu display plugin by connecting to required pieces"""
        print("Menu Display plugin initialized")
        
        # Find the piece manager plugin
        for plugin in self.game_engine.plugin_manager.plugin_instances:
            if hasattr(plugin, 'get_piece_state'):
                self.piece_manager = plugin
                break
        
        # Find menu control plugin to get current menu
        for plugin in self.game_engine.plugin_manager.plugin_instances:
            if hasattr(plugin, 'current_menu'):
                self.control_plugin = plugin
                break
        
        # Initialize display piece to track display state
        if self.piece_manager:
            display_state = self.piece_manager.get_piece_state('display')
            if not display_state:
                display_state = {
                    'last_refresh_time': 0,
                    'refresh_count': 0,
                    'active': True
                }
                self.piece_manager.update_piece_state('display', display_state)
    
    def refresh_display(self):
        """Refresh the menu display"""
        current_menu = 'main_menu'
        
        # Get current menu from control plugin if available
        if self.control_plugin and hasattr(self.control_plugin, 'current_menu'):
            current_menu = self.control_plugin.current_menu
        
        # Get menu state from piece manager
        menu_state = self.piece_manager.get_piece_state(current_menu)
        if not menu_state:
            print(f"Menu '{current_menu}' not found!")
            return
        
        # Update display piece with new refresh data
        display_state = self.piece_manager.get_piece_state('display')
        if display_state:
            import time
            display_state['last_refresh_time'] = str(int(time.time()))
            display_state['refresh_count'] = str(int(display_state.get('refresh_count', 0)) + 1)
            self.piece_manager.update_piece_state('display', display_state)
        
        # Render the menu
        self._render_menu(current_menu, menu_state)
    
    def _render_menu(self, menu_name, menu_state):
        """Render a specific menu"""
        title = menu_state.get('title', menu_name.replace('_', ' ').title())
        option_count = int(menu_state.get('option_count', 0))
        current_selection = int(menu_state.get('current_selection', 0))
        
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
        print("- Use arrow keys to navigate (UP/DOWN to move selection, RIGHT to activate, LEFT to go back)")
        print("- Use Ctrl+C to exit")
        print("- Keyboard input is echoed immediately when pressed")
        print("="*60)
    
    def handle_input(self, key):
        """Handle input events"""
        pass
    
    def update(self, dt):
        """Update display state"""
        pass
    
    def render(self, screen):
        """Render menu interface to screen"""
        # This method is called by the main engine periodically
        # For this implementation, we'll just ensure the display is up-to-date
        pass