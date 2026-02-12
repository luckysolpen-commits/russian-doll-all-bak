"""
Keyboard plugin for the Pet Philosophy project
Handles keyboard input logging to history file following TPM principles
Works with keyboard piece to maintain input logs through master ledger
"""

import os
import time
from plugin_interface import PluginInterface


class Plugin(PluginInterface):
    def __init__(self, game_engine):
        super().__init__(game_engine)
        self.piece_manager = None
        self.command_processor = None
        self.keyboard_piece_name = 'keyboard'
        self.history_file_path = 'pieces/keyboard/history.txt'
        self.master_ledger_file = 'pieces/master_ledger/ledger.txt'
        self.last_processed_position = 0
        
    def initialize(self):
        """Initialize keyboard plugin by connecting to keyboard piece"""
        print("Keyboard plugin initialized")
        
        # Find the piece manager and command processor plugins
        for plugin in self.game_engine.plugin_manager.plugin_instances:
            if hasattr(plugin, 'get_piece_state'):
                self.piece_manager = plugin
            elif hasattr(plugin, 'handle_input') and plugin.__class__.__name__ and 'CommandProcessor' in plugin.__class__.__name__:
                self.command_processor = plugin
        
        # Create history file if it doesn't exist
        os.makedirs(os.path.dirname(self.history_file_path), exist_ok=True)
        if not os.path.exists(self.history_file_path):
            with open(self.history_file_path, 'w') as f:
                f.write("# Keyboard session started\n")
        
        # Create master ledger if it doesn't exist
        os.makedirs(os.path.dirname(self.master_ledger_file), exist_ok=True)
        if not os.path.exists(self.master_ledger_file):
            with open(self.master_ledger_file, 'w') as f:
                f.write("# Master Ledger - Central Audit Trail\n")
        
        # Initialize the session - update piece state
        if self.piece_manager:
            keyboard_state = self.piece_manager.get_piece_state(self.keyboard_piece_name)
            print(f"DEBUG: Loaded keyboard state: {keyboard_state}")
            
            # If keyboard state is empty or None, create a default one
            if not keyboard_state:
                print("DEBUG: Keyboard state was empty, creating default state")
                keyboard_state = {
                    'session_id': int(time.time()),
                    'current_key_log_path': self.history_file_path,
                    'logging_enabled': True,
                    'keys_recorded': 0,
                    'last_key_logged': '',
                    'last_key_time': '',
                    'last_key_code': 0,
                    'status': 'active'
                }
                # Save the default state to make sure it persists
                self.piece_manager.update_piece_state(self.keyboard_piece_name, keyboard_state)
            else:
                # Ensure logging is enabled if the state existed
                keyboard_state['session_id'] = int(time.time())  # Use timestamp as session ID
                keyboard_state['logging_enabled'] = True  # Important: enable logging
                keyboard_state['keys_recorded'] = 0
                self.piece_manager.update_piece_state(self.keyboard_piece_name, keyboard_state)
    
    def log_key(self, key_char):
        """Log a key press to the history file and master ledger"""
        print(f"DEBUG: log_key called with key: '{key_char}' (ASCII: {ord(key_char)})")
        
        if not self.piece_manager:
            print("DEBUG: No piece_manager available")
            return False
            
        # Get current keyboard state
        keyboard_state = self.piece_manager.get_piece_state(self.keyboard_piece_name)
        if not keyboard_state or keyboard_state.get('logging_enabled', False) == False:
            print(f"DEBUG: Keyboard not enabled for logging. State: {keyboard_state}")
            return False
        
        try:
            # Write the key to the history file
            print(f"DEBUG: About to write key {ord(key_char)} to {self.history_file_path}")
            with open(self.history_file_path, 'a') as f:
                f.write(f"{ord(key_char)}\n")  # Write ASCII code like reference implementation
            print(f"DEBUG: Successfully wrote key {ord(key_char)} to history file")
            
            # Update keyboard piece state
            keyboard_state['last_key_logged'] = key_char
            keyboard_state['last_key_code'] = ord(key_char)
            keyboard_state['last_key_time'] = time.strftime('%Y-%m-%d %H:%M:%S')
            keyboard_state['keys_recorded'] = int(keyboard_state.get('keys_recorded', 0)) + 1
            self.piece_manager.update_piece_state(self.keyboard_piece_name, keyboard_state)
            
            # Log to master ledger for audit trail
            timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
            log_entry = f"[{timestamp}] KeyEvent: {key_char} (code {ord(key_char)}) | Source: keyboard\n"
            with open(self.master_ledger_file, 'a') as ledger:
                ledger.write(log_entry)
                print(f"DEBUG: KeyEvent logged to master ledger: {log_entry.strip()}")
                
            return True
        except Exception as e:
            print(f"Error logging key: {e}")
            import traceback
            traceback.print_exc()
            return False
    
    def handle_input(self, key_char):
        """Handle direct input from the main loop - TPM COMPLIANT VERSION
        
        All keyboard inputs MUST go through history.txt first to maintain audit trail,
        then get processed by the master ledger monitor. No direct plugin forwarding.
        """
        if isinstance(key_char, str) and len(key_char) == 1:
            # DEBUG: Print to make sure this is being called
            print(f"DEBUG: KeyboardPlugin handle_input called with key: '{key_char}' (ASCII: {ord(key_char)})")
            
            # Log the key for audit purposes - THIS MUST HAPPEN FIRST for TPM compliance
            success = self.log_key(key_char)
            print(f"DEBUG: log_key returned: {success}")
            
            # In true TPM fashion, we should log method calls to the master ledger
            # for the master ledger monitor to process later, rather than direct calls
            timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
            
            # Log specific keyboard events to the master ledger for the system to process
            if key_char in 'wasd':
                # WASD keys should trigger movement events
                movement_map = {'w': 'key_w', 'a': 'key_a', 's': 'key_s', 'd': 'key_d'}
                event_name = movement_map.get(key_char.lower())
                if event_name:
                    log_entry = f"[{timestamp}] EventFire: {event_name} on mover | Trigger: keyboard_input\n"
                    with open(self.master_ledger_file, 'a') as ledger:
                        ledger.write(log_entry)
                        print(f"DEBUG: EventFire logged for key: {key_char}")
            elif key_char in '123456':
                # Number keys should trigger command processing through the ledger
                command_map = {'1': 'feed', '2': 'play', '3': 'sleep', '4': 'status', '5': 'evolve', '6': 'quit'}
                command_name = command_map.get(key_char)
                if command_name and command_name != 'quit':
                    log_entry = f"[{timestamp}] MethodCall: {command_name} on fuzzpet | Caller: keyboard_input\n"
                    with open(self.master_ledger_file, 'a') as ledger:
                        ledger.write(log_entry)
                        print(f"DEBUG: MethodCall logged for command: {command_name}")
            elif key_char in ['\n', '\r']:
                # Enter key could be for command execution
                log_entry = f"[{timestamp}] EventFire: enter_pressed | Trigger: keyboard_input\n"
                with open(self.master_ledger_file, 'a') as ledger:
                    ledger.write(log_entry)
                    print(f"DEBUG: Enter event logged")