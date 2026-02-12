"""
Action plugin for Roguelike game
Handles interactions with other directories/modules
Implements the 'act' menu functionality from store-summary.txt
"""

from plugin_interface import PluginInterface
import os


class Plugin(PluginInterface):
    def __init__(self, game_engine):
        super().__init__(game_engine)
    
    def initialize(self):
        """Initialize the action system"""
        print("Action plugin initialized")
    
    def handle_action(self):
        """Handle action menu - interact with other directories/modules"""
        print("Executing action...")
        
        # Get list of available directories/modules
        available_dirs = self.find_available_directories()
        
        if not available_dirs:
            print("No directories available to interact with")
            return
        
        print("Available directories:")
        for i, directory in enumerate(available_dirs):
            print(f"  {i}: {directory}")
    
    def find_available_directories(self):
        """Find available directories that can be interacted with"""
        base_dir = "."
        dirs = []
        
        # Look for common directories that may contain game assets
        potential_dirs = [
            "games/default/pieces",
            "games/default/maps",
            "games/default/items",
            "plugins",
            "plugins/core",
            "plugins/ui",
            "data",
            "assets"
        ]
        
        for d in potential_dirs:
            if os.path.exists(d):
                dirs.append(d)
        
        return dirs
    
    def handle_input(self, key):
        """Handle input events"""
        pass
    
    def update(self, dt):
        """Update plugin state"""
        pass
    
    def render(self, screen):
        """Render plugin content"""
        pass