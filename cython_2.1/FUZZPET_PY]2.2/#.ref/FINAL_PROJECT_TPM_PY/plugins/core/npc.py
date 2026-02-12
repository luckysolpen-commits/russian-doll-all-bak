"""
NPC plugin for Roguelike game
Handles AI behaviors for non-player characters like the dog
Works with NPC pieces that have event listeners
"""

from plugin_interface import PluginInterface


class Plugin(PluginInterface):
    def __init__(self, game_engine):
        super().__init__(game_engine)
        self.piece_manager = None
        
    def initialize(self):
        """Initialize NPC plugin"""
        print("NPC plugin initialized")
        
        # Find the piece manager plugin
        for plugin in self.game_engine.plugin_manager.plugin_instances:
            if hasattr(plugin, 'get_piece_state'):
                self.piece_manager = plugin
                break
    
    def respond_to_turn_end(self, npc_obj):
        """Respond to turn_end events - move the NPC based on its behavior pattern"""
        if not self.piece_manager or not hasattr(npc_obj, 'piece_name'):
            return False
            
        piece_name = npc_obj.piece_name
        
        # Get NPC state
        npc_state = self.piece_manager.get_piece_state(piece_name)
        if not npc_state:
            return False
        
        # Simple AI for NPCs - move randomly or follow basic pattern
        import random
        
        # Get current position
        x = int(npc_state.get('x', 0))
        y = int(npc_state.get('y', 0))
        
        # Random move direction
        dx, dy = random.choice([(0, 1), (0, -1), (1, 0), (-1, 0)])
        
        # Check for collisions before moving (would use world plugin here)
        world_plugin = None
        for plugin in self.game_engine.plugin_manager.plugin_instances:
            if hasattr(plugin, 'check_collision'):
                world_plugin = plugin
                break
        
        if world_plugin:
            if not world_plugin.check_collision(x + dx, y + dy):
                # Update position
                npc_state['x'] = x + dx
                npc_state['y'] = y + dy
                self.piece_manager.update_piece_state(piece_name, npc_state)
                print(f"NPC {piece_name} moved to position ({x + dx}, {y + dy})")
        
        return True
    
    def handle_input(self, key):
        """Handle input events"""
        pass
    
    def update(self, dt):
        """Update plugin state"""
        pass
    
    def render(self, screen):
        """Render plugin content"""
        pass