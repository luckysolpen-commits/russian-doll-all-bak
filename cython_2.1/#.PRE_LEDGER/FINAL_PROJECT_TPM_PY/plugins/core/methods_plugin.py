"""
Methods plugin for Roguelike game
Provides reusable behavioral methods for entities that can be referenced from pieces/
Works with piece manager to execute methods on specific pieces
"""

from plugin_interface import PluginInterface


class Plugin(PluginInterface):
    def __init__(self, game_engine):
        super().__init__(game_engine)
        self.piece_manager = None
        
        # Dictionary of available methods that entities can reference
        self.methods = {
            # Game Methods (for general game logic)
            'process_turn': self.method_process_turn,
            'update_stats': self.method_update_stats,
            'increment_turn': self.method_increment_turn,
            'increment_hunger': self.method_increment_hunger,
            
            # State Methods (for managing state)
            'read_piece_state': self.method_read_piece_state,
            'update_piece_state': self.method_update_piece_state,
            'validate_state': self.method_validate_state,
            
            # Entity Movement Methods (for game entities)
            'move_towards_player': self.method_move_towards_player,
            'update_position': self.method_update_position,
            
            # Action Methods (for performing actions)
            'perform_attack': self.method_perform_attack,
            'take_damage': self.method_take_damage,
            'heal': self.method_heal,
            
            # Time Methods (for time-related logic)
            'tick': self.method_tick,
            'get_time': self.method_get_time,
        }
    
    def initialize(self):
        """Initialize methods plugin"""
        print("Methods plugin initialized")
        
        # Find the piece manager plugin
        for plugin in self.game_engine.plugin_manager.plugin_instances:
            if hasattr(plugin, 'get_piece_state'):
                self.piece_manager = plugin
                break
        
        print(f"Methods plugin loaded with {len(self.methods)} methods")
    
    def method_process_turn(self, piece_obj):
        """Process a game turn for the given piece"""
        print("Processing turn...")
        return True
    
    def method_update_stats(self, piece_obj):
        """Update statistics for the given piece"""
        print("Updating stats...")
        return True
    
    def method_increment_turn(self, piece_obj):
        """Increment turn counter for the given piece"""
        if hasattr(piece_obj, 'state'):
            current_turn = int(piece_obj.state.get('turn', 0))
            piece_obj.state['turn'] = current_turn + 1
            print(f"Turn incremented to: {current_turn + 1}")
            return True
        else:
            print("Object doesn't have state to increment turn")
            return False
    
    def method_increment_hunger(self, piece_obj):
        """Increment hunger for the given piece"""
        if hasattr(piece_obj, 'state'):
            current_hunger = int(piece_obj.state.get('hunger', 0))
            piece_obj.state['hunger'] = current_hunger + 1
            print(f"Hunger incremented to: {current_hunger + 1}")
            return True
        else:
            print("Object doesn't have state to increment hunger")
            return False
    
    def method_read_piece_state(self, piece_obj):
        """Read state of the given piece"""
        if hasattr(piece_obj, 'state'):
            print(f"Piece state: {piece_obj.state}")
            return True
        else:
            print("Object doesn't have state to read")
            return False
    
    def method_update_piece_state(self, piece_obj):
        """Update state of the given piece"""
        print("Updating piece state...")
        return True
    
    def method_validate_state(self, piece_obj):
        """Validate state of the given piece"""
        print("Validating state...")
        return True
    
    def method_move_towards_player(self, piece_obj):
        """Move the piece towards the player"""
        print("Moving towards player...")
        return True
    
    def method_update_position(self, piece_obj):
        """Update the position of the given piece"""
        if hasattr(piece_obj, 'state'):
            x = int(piece_obj.state.get('x', 0))
            y = int(piece_obj.state.get('y', 0))
            # Example: move one space right
            piece_obj.state['x'] = x + 1
            print(f"Position updated to: ({x + 1}, {y})")
            return True
        else:
            print("Object doesn't have state to update position")
            return False
    
    def method_perform_attack(self, piece_obj):
        """Perform attack for the given piece"""
        print("Performing attack...")
        return True
    
    def method_take_damage(self, piece_obj):
        """Apply damage to the given piece"""
        if hasattr(piece_obj, 'state'):
            current_health = int(piece_obj.state.get('health', 100))
            new_health = max(0, current_health - 10)  # Deal 10 damage
            piece_obj.state['health'] = new_health
            print(f"Taking 10 damage, health now: {new_health}")
            return True
        else:
            print("Object doesn't have state to take damage")
            return False
    
    def method_heal(self, piece_obj):
        """Heal the given piece"""
        if hasattr(piece_obj, 'state'):
            current_health = int(piece_obj.state.get('health', 100))
            new_health = min(100, current_health + 10)  # Heal 10 points
            piece_obj.state['health'] = new_health
            print(f"Healing 10 points, health now: {new_health}")
            return True
        else:
            print("Object doesn't have state to heal")
            return False
    
    def method_tick(self, piece_obj):
        """Time tick method"""
        if hasattr(piece_obj, 'state'):
            current_time = int(piece_obj.state.get('current_time', 0))
            piece_obj.state['current_time'] = current_time + 1
            print(f"Time tick: {current_time + 1}")
            return True
        else:
            print("Object doesn't have state for time tick")
            return False
    
    def method_get_time(self, piece_obj):
        """Get current time"""
        if hasattr(piece_obj, 'state'):
            current_time = piece_obj.state.get('current_time', 0)
            print(f"Current time: {current_time}")
            return True
        else:
            print("Object doesn't have state for time")
            return False
    
    def handle_input(self, key):
        """Handle input events"""
        pass
    
    def update(self, dt):
        """Update plugin state"""
        pass
    
    def render(self, screen):
        """Render plugin content"""
        pass