"""
Methods plugin for the Pet Philosophy project
Provides reusable behavioral methods for pet following TPM principles
"""

from plugin_interface import PluginInterface


class Plugin(PluginInterface):
    def __init__(self, game_engine):
        super().__init__(game_engine)
        self.piece_manager = None
        
        # Dictionary of available methods that entities can reference
        self.methods = {
            # Health/Hunger Methods
            'feed': self.method_feed,
            'increment_hunger': self.method_increment_hunger,
            
            # Happiness/Energy Methods
            'play': self.method_play,
            'sleep': self.method_sleep,
            
            # Status Methods
            'status': self.method_status,
            'evolve': self.method_evolve,
            
            # State Methods
            'read_piece_state': self.method_read_piece_state,
            'update_piece_state': self.method_update_piece_state,
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
    
    def method_feed(self, piece_obj):
        """Feed the pet - reduce hunger"""
        if hasattr(piece_obj, 'state'):
            hunger = int(piece_obj.state.get('hunger', 0))
            new_hunger = max(0, hunger - 20)
            piece_obj.state['hunger'] = new_hunger
            
            happiness = int(piece_obj.state.get('happiness', 55))
            new_happiness = min(100, happiness + 10)
            piece_obj.state['happiness'] = new_happiness
            
            print(f"Pet fed! Hunger reduced from {hunger} to {new_hunger}")
            return {'hunger': new_hunger, 'happiness': new_happiness}
        return {}
    
    def method_increment_hunger(self, piece_obj):
        """Increment the pet's hunger"""
        if hasattr(piece_obj, 'state'):
            hunger = int(piece_obj.state.get('hunger', 0))
            new_hunger = min(100, hunger + 5)
            piece_obj.state['hunger'] = new_hunger
            print(f"Pet hunger increased from {hunger} to {new_hunger}")
            return {'hunger': new_hunger}
        return {}
    
    def method_play(self, piece_obj):
        """Play with the pet - increase happiness"""
        if hasattr(piece_obj, 'state'):
            happiness = int(piece_obj.state.get('happiness', 55))
            new_happiness = min(100, happiness + 15)
            energy = int(piece_obj.state.get('energy', 100))
            new_energy = max(0, energy - 10)
            
            piece_obj.state['happiness'] = new_happiness
            piece_obj.state['energy'] = new_energy
            
            print(f"Pet played with! Happiness increased to {new_happiness}, energy decreased to {new_energy}")
            return {'happiness': new_happiness, 'energy': new_energy}
        return {}
    
    def method_sleep(self, piece_obj):
        """Let the pet sleep - restore energy"""
        if hasattr(piece_obj, 'state'):
            energy = int(piece_obj.state.get('energy', 100))
            new_energy = min(100, energy + 25)
            happiness = int(piece_obj.state.get('happiness', 55))
            new_happiness = min(100, happiness + 5)
            
            piece_obj.state['energy'] = new_energy
            piece_obj.state['happiness'] = new_happiness
            
            print(f"Pet slept! Energy restored to {new_energy}, happiness increased to {new_happiness}")
            return {'energy': new_energy, 'happiness': new_happiness}
        return {}
    
    def method_status(self, piece_obj):
        """Show pet status"""
        if hasattr(piece_obj, 'state'):
            state = piece_obj.state
            print(f"Pet Status: Name: {state.get('name', 'Unknown')}, "
                  f"Hunger: {state.get('hunger', 'Unknown')}, "
                  f"Happiness: {state.get('happiness', 'Unknown')}, "
                  f"Energy: {state.get('energy', 'Unknown')}, "
                  f"Level: {state.get('level', 'Unknown')}")
        return {}
    
    def method_evolve(self, piece_obj):
        """Evolve the pet to next level"""
        if hasattr(piece_obj, 'state'):
            level = int(piece_obj.state.get('level', 1))
            new_level = min(10, level + 1)  # Cap at level 10
            happiness = int(piece_obj.state.get('happiness', 55))
            new_happiness = min(100, happiness + 10)
            
            piece_obj.state['level'] = new_level
            piece_obj.state['happiness'] = new_happiness
            
            print(f"Pet evolved! Level increased to {new_level}, happiness increased to {new_happiness}")
            return {'level': new_level, 'happiness': new_happiness}
        return {}
    
    def method_read_piece_state(self, piece_obj):
        """Read the state of a piece"""
        if hasattr(piece_obj, 'state'):
            return piece_obj.state
        return {}
    
    def method_update_piece_state(self, piece_obj):
        """Update a piece's state (placeholder - actual updates happen elsewhere)"""
        return {}