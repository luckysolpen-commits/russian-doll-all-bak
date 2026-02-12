"""
Health plugin for the Pet Philosophy project
Manages pet health and hunger following TPM principles
"""

from plugin_interface import PluginInterface


class Plugin(PluginInterface):
    def __init__(self, game_engine):
        super().__init__(game_engine)
        self.piece_manager = None
        self.pet_piece_name = 'fuzzpet'
        
    def initialize(self):
        """Initialize the health system"""
        print("Health plugin initialized")
        
        # Find the piece manager plugin
        for plugin in self.game_engine.plugin_manager.plugin_instances:
            if hasattr(plugin, 'get_piece_state'):
                self.piece_manager = plugin
                break
    
    def increment_hunger(self):
        """Increment pet hunger based on passage of time"""
        if not self.piece_manager:
            return
            
        pet_state = self.piece_manager.get_piece_state(self.pet_piece_name)
        if not pet_state or 'state' not in pet_state:
            return
            
        current_hunger = int(pet_state['state'].get('hunger', 0))
        new_hunger = min(100, current_hunger + 1)  # Cap at 100
        
        pet_state['state']['hunger'] = new_hunger
        
        # Update the piece state
        self.piece_manager.update_piece_state(self.pet_piece_name, pet_state)
        
        # Log the hunger change to the master ledger
        import time
        timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
        log_entry = f"[{timestamp}] StateChange: fuzzpet hunger {new_hunger} | Trigger: hunger_increase\n"
        with open('pieces/master_ledger/ledger.txt', 'a') as ledger:
            ledger.write(log_entry)
        
        # If hunger gets too high, trigger negative events
        if new_hunger > 80:
            print("[PET] The pet is very hungry!")
            
            # Trigger hunger event
            event_entry = f"[{timestamp}] EventFire: very_hungry on fuzzpet | Trigger: hunger_threshold\n"
            with open('pieces/master_ledger/ledger.txt', 'a') as ledger:
                ledger.write(event_entry)
    
    def update(self, dt):
        """Update plugin state - increment hunger over time"""
        # Increment hunger gradually over time (every few seconds)
        if hasattr(self, '_hunger_timer'):
            self._hunger_timer += dt
            if self._hunger_timer >= 5.0:  # Increment hunger every 5 seconds
                self.increment_hunger()
                self._hunger_timer = 0.0
        else:
            self._hunger_timer = 0.0