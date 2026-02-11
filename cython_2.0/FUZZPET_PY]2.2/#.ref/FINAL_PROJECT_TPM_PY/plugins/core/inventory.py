"""
Inventory plugin for Roguelike game
Manages player inventory items stored in bag directory
Works with player piece and its bag subdirectories
"""

import os
from plugin_interface import PluginInterface


class Plugin(PluginInterface):
    def __init__(self, game_engine):
        super().__init__(game_engine)
        self.piece_manager = None
        self.player_piece_name = 'player1'
        
    def initialize(self):
        """Initialize inventory plugin"""
        print("Inventory plugin initialized")
        
        # Find the piece manager plugin
        for plugin in self.game_engine.plugin_manager.plugin_instances:
            if hasattr(plugin, 'get_piece_state'):
                self.piece_manager = plugin
                break
    
    def handle_inventory(self):
        """Handle inventory menu option - browse player's bag directory"""
        print("Opening inventory...")
        
        # Look for items in player's inventory (would be in a bag subdirectory)
        inventory_dir = f"games/default/pieces/{self.player_piece_name}/bag"
        if os.path.exists(inventory_dir):
            items = os.listdir(inventory_dir)
            print(f"Items in {self.player_piece_name}'s bag ({len(items)} total):")
            for i, item in enumerate(items):
                print(f"  {i}: {item}")
        else:
            print("No inventory found for player")
    
    def handle_input(self, key):
        """Handle input events"""
        pass
    
    def update(self, dt):
        """Update plugin state"""
        pass
    
    def render(self, screen):
        """Render plugin content"""
        pass