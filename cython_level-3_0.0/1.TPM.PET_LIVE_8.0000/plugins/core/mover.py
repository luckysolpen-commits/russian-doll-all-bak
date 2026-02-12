"""
Mover plugin for the Pet Philosophy project
Handles movement based on keyboard input and updates pet position
Follows TPM architecture by processing movement events through master ledger
"""

from plugin_interface import PluginInterface


class Plugin(PluginInterface):
    def __init__(self, game_engine):
        super().__init__(game_engine)
        self.piece_manager = None
        self.mover_piece_name = 'mover'
        self.master_ledger_file = 'pieces/master_ledger/ledger.txt'
        
    def initialize(self):
        """Initialize mover plugin by connecting to required pieces"""
        print("Mover plugin initialized")
        
        # Find the piece manager plugin
        for plugin in self.game_engine.plugin_manager.plugin_instances:
            if hasattr(plugin, 'get_piece_state'):
                self.piece_manager = plugin
                break
        
        # Initialize the mover piece
        if self.piece_manager:
            # Create mover state if it doesn't exist
            mover_state = self.piece_manager.get_piece_state(self.mover_piece_name)
            if not mover_state:
                mover_state = {
                    'position_x': 0,
                    'position_y': 0,
                    'last_direction': 'none',
                    'status': 'active'
                }
                self.piece_manager.update_piece_state(self.mover_piece_name, mover_state)
    
    def handle_input(self, key_char):
        """Handle input events - TPM COMPLIANT: Moved to master ledger processing
        
        All movement events should be triggered through the master ledger,
        not through direct keyboard input handling.
        """
        # No direct keyboard handling - events should come through master ledger
        pass
    
    def process_movement_key(self, key_char):
        """Process a movement key press directly"""
        if not self.piece_manager:
            return
            
        mover_state = self.piece_manager.get_piece_state(self.mover_piece_name)
        if not mover_state:
            return
            
        # Store original position for event logging
        old_x, old_y = int(mover_state.get('position_x', 0)), int(mover_state.get('position_y', 0))
        
        # Update position based on key
        movement_output = []
        if key_char == 'w':
            mover_state['position_y'] = int(mover_state.get('position_y', 0)) - 1
            mover_state['last_direction'] = 'up'
            movement_output.append(f"[MOVER] Moving up! New position: ({mover_state['position_x']}, {mover_state['position_y']})")
        elif key_char == 's':
            mover_state['position_y'] = int(mover_state.get('position_y', 0)) + 1
            mover_state['last_direction'] = 'down'
            movement_output.append(f"[MOVER] Moving down! New position: ({mover_state['position_x']}, {mover_state['position_y']})")
        elif key_char == 'a':
            mover_state['position_x'] = int(mover_state.get('position_x', 0)) - 1
            mover_state['last_direction'] = 'left'
            movement_output.append(f"[MOVER] Moving left! New position: ({mover_state['position_x']}, {mover_state['position_y']})")
        elif key_char == 'd':
            mover_state['position_x'] = int(mover_state.get('position_x', 0)) + 1
            mover_state['last_direction'] = 'right'
            movement_output.append(f"[MOVER] Moving right! New position: ({mover_state['position_x']}, {mover_state['position_y']})")
        
        # Update piece state
        self.piece_manager.update_piece_state(self.mover_piece_name, mover_state)
        
        # Also update the fuzzpet piece with the new position
        fuzzpet_state = self.piece_manager.get_piece_state('fuzzpet')
        if fuzzpet_state:
            if 'state' not in fuzzpet_state:
                fuzzpet_state['state'] = {}
            fuzzpet_state['state']['pos_x'] = int(mover_state.get('position_x', 0))
            fuzzpet_state['state']['pos_y'] = int(mover_state.get('position_y', 0))
            self.piece_manager.update_piece_state('fuzzpet', fuzzpet_state)
        
        # Log to master ledger for audit trail
        import time
        timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
        log_entry = f"[{timestamp}] MovementEvent: {mover_state['last_direction']} | From: ({old_x},{old_y}) To: ({mover_state['position_x']},{mover_state['position_y']}) | Target: mover\n"
        with open(self.master_ledger_file, 'a') as ledger:
            ledger.write(log_entry)
        
        # Store movement output for display
        command_processor = self.game_engine.plugin_manager.find_plugin_by_capability('last_command_output')
        if command_processor and hasattr(command_processor, 'last_command_output'):
            command_processor.last_command_output = movement_output
        
        # Create an event that could be listened to by other pieces
        event_name = f"key_{key_char}"
        self.trigger_event(event_name, 'mover')
        
        # Note: Display updates are now handled by the master ledger monitor when events are processed
        # removing the direct call here to prevent feedback loops
    
    def trigger_event(self, event_name, piece_id):
        """Simulate triggering an event (future event manager would handle this)"""
        # For now, just log to master ledger
        # NOTE: We should be careful not to create events that would cause infinite loops
        import time
        timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
        # Only log non-movement events to prevent infinite loops with WASD keys
        if not event_name.startswith('key_'):
            log_entry = f"[{timestamp}] EventFire: {event_name} on {piece_id} | Trigger: movement_key_press\n"
            with open(self.master_ledger_file, 'a') as ledger:
                ledger.write(log_entry)
    
    def update(self, dt):
        """Check for recent key inputs from keyboard history"""
        # This method would normally check the keyboard history file for new inputs
        # But since we handle input directly in handle_input, this is just for completeness
        pass