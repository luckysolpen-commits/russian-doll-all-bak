"""
Command Processor plugin for the Pet Philosophy project
Interprets number commands (1-6) for pet actions following TPM principles
"""

from plugin_interface import PluginInterface


class Plugin(PluginInterface):
    def __init__(self, game_engine):
        super().__init__(game_engine)
        self.piece_manager = None
        self.command_processor_piece_name = 'command_processor'
        self.fuzzpet_piece_name = 'fuzzpet'
        self.master_ledger_file = 'pieces/master_ledger/ledger.txt'
        self.command_map = {
            '1': 'feed',
            '2': 'play', 
            '3': 'sleep',
            '4': 'status',
            '5': 'evolve',
            '6': 'quit'
        }
        self.command_buffer = ""
        self.last_command_output = []  # Store recent command output
        
    def initialize(self):
        """Initialize command processor plugin"""
        print("Command Processor plugin initialized")
        
        # Find the piece manager plugin
        for plugin in self.game_engine.plugin_manager.plugin_instances:
            if hasattr(plugin, 'get_piece_state'):
                self.piece_manager = plugin
                break
        
        # Initialize the command processor piece
        if self.piece_manager:
            # Create command processor state if it doesn't exist
            processor_state = self.piece_manager.get_piece_state(self.command_processor_piece_name)
            if not processor_state:
                processor_state = {
                    'current_command': '',
                    'command_buffer': '',
                    'command_mode': 'waiting',
                    'status': 'active'
                }
                self.piece_manager.update_piece_state(self.command_processor_piece_name, processor_state)
    
    def handle_input(self, key_char):
        """Handle command input - process number keys for pet commands"""
        if key_char in self.command_map:
            command = self.command_map[key_char]
            if command != 'quit':
                self.execute_command(command)
            else:
                print(f"\n[COMMAND] Quit command received. Would exit in full implementation.")
        elif key_char == '\n' or key_char == '\r':  # Enter key
            if self.command_buffer:
                # Execute command from buffer
                command = self.command_buffer.strip()
                if command in self.command_map.values():
                    self.execute_command(command)
                self.command_buffer = ""
                
                # Update command processor state
                if self.piece_manager:
                    processor_state = self.piece_manager.get_piece_state(self.command_processor_piece_name)
                    if processor_state:
                        processor_state['command_buffer'] = self.command_buffer
                        processor_state['current_command'] = ''
                        self.piece_manager.update_piece_state(self.command_processor_piece_name, processor_state)
        elif key_char.isdigit():
            # Add digit to command buffer for multi-key commands in future
            self.command_buffer += key_char
            if self.piece_manager:
                processor_state = self.piece_manager.get_piece_state(self.command_processor_piece_name)
                if processor_state:
                    processor_state['command_buffer'] = self.command_buffer
                    self.piece_manager.update_piece_state(self.command_processor_piece_name, processor_state)
    
    def execute_command(self, command_name):
        """Execute a pet command by interacting with the fuzzpet piece"""
        command_output = [f"[COMMAND] Executing: {command_name}"]
        
        if self.piece_manager:
            # Get current fuzzpet state
            fuzzpet_state = self.piece_manager.get_piece_state(self.fuzzpet_piece_name)
            if fuzzpet_state:
                # Ensure state exists
                if 'state' not in fuzzpet_state:
                    fuzzpet_state['state'] = {}
                
                # Simulate command execution - in a full implementation, this would call real plugin methods
                if command_name == 'feed':
                    old_hunger = int(fuzzpet_state['state'].get('hunger', 30))
                    new_hunger = max(0, int(old_hunger) - 20)
                    fuzzpet_state['state']['hunger'] = new_hunger
                    command_output.append(f"[PET] Feeding FuzzPet! Hunger decreased from {old_hunger} to {new_hunger}")
                    
                    # Trigger fed event
                    self.trigger_event('fed')
                    
                elif command_name == 'play':
                    old_happiness = int(fuzzpet_state['state'].get('happiness', 55))
                    new_happiness = min(100, int(old_happiness) + 15)
                    fuzzpet_state['state']['happiness'] = new_happiness
                    command_output.append(f"[PET] Playing with FuzzPet! Happiness increased from {old_happiness} to {new_happiness}")
                    
                    # Trigger played event
                    self.trigger_event('played')
                    
                elif command_name == 'sleep':
                    old_energy = int(fuzzpet_state['state'].get('energy', 100))
                    new_energy = min(100, int(old_energy) + 20)
                    fuzzpet_state['state']['energy'] = new_energy
                    command_output.append(f"[PET] FuzzPet sleeping! Energy increased from {old_energy} to {new_energy}")
                    
                    # Trigger slept event
                    self.trigger_event('slept')
                    
                elif command_name == 'status':
                    state_vals = fuzzpet_state['state']
                    command_output.append(f"[PET STATUS] Name: {state_vals.get('name', 'FuzzPet')}, "
                                          f"Hunger: {state_vals.get('hunger', 0)}, "
                                          f"Happiness: {state_vals.get('happiness', 0)}, "
                                          f"Energy: {state_vals.get('energy', 0)}, "
                                          f"Level: {state_vals.get('level', 1)}")
                    
                elif command_name == 'evolve':
                    old_level = int(fuzzpet_state['state'].get('level', 1))
                    new_level = min(10, int(old_level) + 1)  # Cap at level 10
                    fuzzpet_state['state']['level'] = new_level
                    command_output.append(f"[PET] FuzzPet evolved! Level increased from {old_level} to {new_level}")
                    
                    # Trigger evolve event
                    self.trigger_event('evolve')
                
                # Update fuzzpet piece state
                self.piece_manager.update_piece_state(self.fuzzpet_piece_name, fuzzpet_state)
                
                # Log command execution to master ledger
                import time
                timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
                log_entry = f"[{timestamp}] CommandExecuted: {command_name} on fuzzpet | Source: command_processor\n"
                with open(self.master_ledger_file, 'a') as ledger:
                    ledger.write(log_entry)
                
                # Store the command output for display
                self.last_command_output = command_output
                
                # Clear the screen and update display after command execution
                for plugin in self.game_engine.plugin_manager.plugin_instances:
                    if hasattr(plugin, 'update_display'):
                        plugin.update_display()
    
    def trigger_event(self, event_name):
        """Trigger an event that other pieces can respond to"""
        import time
        timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
        
        # Log event to master ledger
        log_entry = f"[{timestamp}] EventFire: {event_name} on fuzzpet | Trigger: command_execution\n"
        with open(self.master_ledger_file, 'a') as ledger:
            ledger.write(log_entry)
        
        # Print response based on event (in a full system, event manager would handle this)
        responses = {
            'fed': 'Yum yum~ Feeling so happy! *tail wags wildly*',
            'played': 'Wheee~ Best playtime ever! *bounces around joyfully*',
            'slept': 'Zzz... all cozy now~ *yawns cutely* Feeling recharged!',
            'evolve': '*glows brightly* Level up! I\'m getting stronger~!'
        }
        
        if event_name in responses:
            print(f"[PET] {responses[event_name]}")
            
            # Also log the response to master ledger
            response_entry = f"[{timestamp}] Response: fuzzpet {event_name} | Message: {responses[event_name]}\n"
            with open(self.master_ledger_file, 'a') as ledger:
                ledger.write(response_entry)