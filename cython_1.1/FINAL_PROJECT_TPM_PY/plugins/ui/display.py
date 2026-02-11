"""
Display plugin for ASCII Roguelike game
Renders both the ASCII map and menu system in a unified interface
"""

from plugin_interface import PluginInterface


class Plugin(PluginInterface):
    def __init__(self, game_engine):
        super().__init__(game_engine)
        self.piece_manager = None
        self.world_plugin = None
        self.player_plugin = None
        self.control_plugin = None
        self.display_piece = None
        
    def initialize(self):
        """Initialize display plugin by connecting to required pieces"""
        print("Display plugin initialized")
        
        # Find the piece manager plugin
        for plugin in self.game_engine.plugin_manager.plugin_instances:
            if hasattr(plugin, 'get_piece_state'):
                self.piece_manager = plugin
                break
        
        # Find other relevant plugins
        for plugin in self.game_engine.plugin_manager.plugin_instances:
            if hasattr(plugin, 'get_display_map_with_entities'):
                self.world_plugin = plugin
            elif hasattr(plugin, 'get_player_position'):
                self.player_plugin = plugin
            elif hasattr(plugin, 'handle_input'):
                self.control_plugin = plugin
        
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
    
    def handle_input(self, key):
        """Handle input events"""
        pass
    
    def update(self, dt):
        """Update display state"""
        pass
    
    def render(self, screen=None):
        """Render ASCII map and menu to screen - only when needed"""
        pass  # We'll control rendering externally instead of always rendering
    
    def refresh_display(self):
        """Refresh the display when there's a meaningful change"""
        # Get the map with entities and player
        map_display = None
        if self.world_plugin and hasattr(self.world_plugin, 'get_display_map_with_entities'):
            map_display = self.world_plugin.get_display_map_with_entities()
        
        # Get player info
        player_pos = None
        if self.player_plugin and hasattr(self.player_plugin, 'get_player_position'):
            player_pos = self.player_plugin.get_player_position()
        
        # Get menu state
        menu_state = None
        if self.piece_manager:
            menu_state = self.piece_manager.get_piece_state('menu')
        
        # Get last key pressed state
        control_state = None
        if self.piece_manager:
            control_state = self.piece_manager.get_piece_state('keyboard')
        
        # Print everything - first the map
        print("\033[H\033[J", end='')  # Clear screen (ANSI code)
        
        if map_display:
            print("ASCII MAP:")
            print("=" * 80)
            for row in map_display:
                print(''.join(row))
            print("=" * 80)
        
        # Then player info
        if player_pos:
            print(f"Player position: ({player_pos[0]}, {player_pos[1]})")
        
        # Then menu options
        if menu_state:
            print(f"\nMENU OPTIONS:")
            option_count = int(menu_state.get('option_count', 0))
            for i in range(option_count):
                option_text = menu_state.get(f'option_{i}', f'Option {i}')
                print(f"{i}: {option_text}")
        
        # Check if there was a menu selection
        if control_state and 'last_menu_choice' in control_state:
            choice = control_state.get('last_menu_choice')
            if choice is not None:
                print(f"You selected menu option: {choice}")
                # Reset the choice after displaying
                control_state['last_menu_choice'] = None
                self.piece_manager.update_piece_state('keyboard', control_state)
        
        # Add a legend for map symbols
        print("\nLEGEND: @=Player, #=Wall, .=Ground, T=Tree, ~=Water, R=Rock, Z=Zombie, &=Sheep")
        print("CONTROLS: WASD or arrows to move, 0-9 for menu options, Ctrl+C to quit")