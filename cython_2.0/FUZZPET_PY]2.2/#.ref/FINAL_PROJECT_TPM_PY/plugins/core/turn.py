"""
Turn plugin for Roguelike game
Manages game turns using methods from methods plugin
"""

from plugin_interface import PluginInterface


class Plugin(PluginInterface):
    def __init__(self, game_engine):
        super().__init__(game_engine)
    
    def initialize(self):
        """Initialize the turn system"""
        print("Turn plugin initialized")
    
    def increment_turn(self):
        """Find player plugin and increment turn"""
        player_plugin = None
        # Access the plugin instances through the game_engine's plugin_manager
        for plugin in self.game_engine.plugin_manager.plugin_instances:
            if hasattr(plugin, 'increment_turn'):
                player_plugin = plugin
                break
        
        if player_plugin:
            # Use the increment_turn method from the player plugin
            player_plugin.increment_turn()
        else:
            print("Could not find player plugin with increment_turn method")
    
    def handle_input(self, key):
        """Handle input events"""
        pass
    
    def update(self, dt):
        """Update plugin state"""
        pass
    
    def render(self, screen):
        """Render plugin content"""
        pass