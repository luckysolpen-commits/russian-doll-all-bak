"""
Health plugin for Roguelike game
Coordinates health and hunger management using methods from methods plugin
"""

from plugin_interface import PluginInterface


class Plugin(PluginInterface):
    def __init__(self, game_engine):
        super().__init__(game_engine)
    
    def initialize(self):
        """Initialize the health system"""
        print("Health plugin initialized")
    
    def increment_hunger(self):
        """Delegate to player plugin to increment hunger"""
        # Find player plugin and delegate to it
        player_plugin = None
        for plugin in self.game_engine.plugin_manager.plugin_instances:
            if hasattr(plugin, 'increment_hunger'):
                player_plugin = plugin
                break
        
        if player_plugin:
            player_plugin.increment_hunger()
        else:
            print("Could not find player plugin with increment_hunger method")
    
    def handle_input(self, key):
        """Handle input events"""
        pass
    
    def update(self, dt):
        """Update plugin state"""
        pass
    
    def render(self, screen):
        """Render plugin content"""
        pass