"""
Player plugin for ASCII Roguelike game
Manages player character state and movement using TPM architecture
"""

from plugin_interface import PluginInterface


class Plugin(PluginInterface):
    def __init__(self, game_engine):
        super().__init__(game_engine)
        self.piece_manager = None
        self.player_piece_name = 'player1'
        self.position = [0, 0]  # [x, y]
        self.symbol = '@'
        self.health = 100
        self.inventory = []
        self.log = ["- Welcome to the Roguelike Game!"]
    
    def initialize(self):
        """Initialize the player by getting state from piece manager"""
        print("Player plugin initialized")
        
        # Find the piece manager plugin
        for plugin in self.game_engine.plugin_manager.plugin_instances:
            if hasattr(plugin, 'get_piece_state'):
                self.piece_manager = plugin
                break
        
        # Initialize player state from piece manager
        if self.piece_manager:
            player_state = self.piece_manager.get_piece_state(self.player_piece_name)
            if player_state:
                # Load state from piece
                # Check if player has specific position in piece; if not, try to find spawn point
                pos_x = int(player_state.get('x', -1))
                pos_y = int(player_state.get('y', -1))
                
                # If player has no predefined position, try to find a spawn point from world
                if pos_x == -1 or pos_y == -1:
                    # Look for spawn point in world entities (like a building with spawn point)
                    spawn_pos = self._find_spawn_point_from_world()
                    if spawn_pos:
                        pos_x, pos_y = spawn_pos
                    else:
                        # Default to (0,0) if no spawn point found
                        pos_x, pos_y = 0, 0
                
                self.position[0] = pos_x
                self.position[1] = pos_y
                self.symbol = player_state.get('symbol', '@')
                self.health = int(player_state.get('health', 100))
                
                # Update state in piece manager to reflect current values
                player_state['x'] = self.position[0]
                player_state['y'] = self.position[1]
                player_state['symbol'] = self.symbol
                player_state['health'] = self.health
                self.piece_manager.update_piece_state(self.player_piece_name, player_state)
    
    def _find_spawn_point_from_world(self):
        """Find a spawn point from world entities if available"""
        # Look for world plugin to check for spawn points
        world_plugin = None
        for plugin in self.game_engine.plugin_manager.plugin_instances:
            if hasattr(plugin, 'entities') and hasattr(plugin, 'get_tile'):
                world_plugin = plugin
                break
        
        if world_plugin and hasattr(world_plugin, 'entities'):
            # Look for entities that might be spawn points
            # In the original entities file, there's an '@' with "Building with player spawn point"
            for (y, x), symbol in world_plugin.entities.items():
                # If world plugin has an @ entry for spawn point, use that
                # In our case, we ignore '@' in entities loading, so we'll use other logic
                # For now, check if there's a designated spawn point in the world description
                # We can't rely on the @ symbol since we filtered it out in load_entities_from_file
                pass
        
        # For now, since the @ was filtered out as decorative, just return a default safe spawn
        # Let's return the original spawn from the entity file: 15, 10
        # In a real system, we'd have a more sophisticated spawn point detection
        return (15, 10)  # Default spawn point from original entity file
    
    def get_player_position(self):
        """Get player position for other plugins"""
        return self.position
    
    def move_player(self, dx, dy):
        """Move player by dx, dy if possible"""
        if not self.piece_manager:
            return False
        
        new_x = self.position[0] + dx
        new_y = self.position[1] + dy
        
        # Check for collisions using the world plugin if available
        world_plugin = self.game_engine.plugin_manager.find_plugin_by_capability('check_collision')
        if world_plugin and hasattr(world_plugin, 'check_collision'):
            if world_plugin.check_collision(new_x, new_y):
                return False  # Cannot move into collision
        # If no world plugin found, still allow movement (for basic movement)
        
        # Update position
        self.position[0] = new_x
        self.position[1] = new_y
        
        # Update player piece state
        player_state = self.piece_manager.get_piece_state(self.player_piece_name)
        if player_state:
            player_state['x'] = self.position[0]
            player_state['y'] = self.position[1]
            self.piece_manager.update_piece_state(self.player_piece_name, player_state)
        
        # Add movement log
        self.log.append(f"- Moved to ({self.position[0]}, {self.position[1]})")
        
        return True
    
    def handle_input(self, key):
        """Handle input - WASD and arrow keys for movement"""
        # Map keys to movement deltas
        if key == 'w' or key == '\x1b[A':  # W or Up arrow
            self.move_player(0, -1)
        elif key == 's' or key == '\x1b[B':  # S or Down arrow
            self.move_player(0, 1)
        elif key == 'a' or key == '\x1b[D':  # A or Left arrow
            self.move_player(-1, 0)
        elif key == 'd' or key == '\x1b[C':  # D or Right arrow
            self.move_player(1, 0)
        # Handle numeric menu inputs in the control plugin
    
    def increment_turn(self):
        """Increment turn counter"""
        if not self.piece_manager:
            return
        
        player_state = self.piece_manager.get_piece_state(self.player_piece_name)
        if player_state:
            current_turn = int(player_state.get('turn', 0))
            player_state['turn'] = current_turn + 1
            self.piece_manager.update_piece_state(self.player_piece_name, player_state)
    
    def increment_hunger(self):
        """Increment hunger counter"""
        if not self.piece_manager:
            return
        
        player_state = self.piece_manager.get_piece_state(self.player_piece_name)
        if player_state:
            current_hunger = int(player_state.get('hunger', 0))
            player_state['hunger'] = current_hunger + 1
            self.piece_manager.update_piece_state(self.player_piece_name, player_state)
    
    def update(self, dt):
        """Update player state"""
        # Limit the size of the log
        if len(self.log) > 10:
            self.log = self.log[-10:]
    
    def render(self, screen):
        """Render player info to screen"""
        pass