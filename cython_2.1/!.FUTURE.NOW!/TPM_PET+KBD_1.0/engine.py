"""
TPM Foundation - Keyboard Input with Master Ledger
Demonstrates immediate keyboard input flowing through piece system and master ledger
"""

import sys
import tty
import termios
import select
import time
import os
from pathlib import Path


class PluginInterface:
    """Base interface that all plugins must implement"""
    def __init__(self, game_engine):
        self.game_engine = game_engine
        self.enabled = True
    
    def initialize(self):
        """Called when the plugin is loaded"""
        pass
    
    def activate(self):
        """Called when the plugin is activated"""
        self.enabled = True
    
    def deactivate(self):
        """Called when the plugin is deactivated""" 
        self.enabled = False
    
    def handle_input(self, key):
        """Handle input events"""
        pass
    
    def update(self, dt):
        """Update plugin state"""
        pass
    
    def render(self, screen):
        """Render plugin content to screen"""
        pass


class PluginManager:
    """Manages loading, initializing, and coordinating plugins"""
    
    def __init__(self):
        self.plugin_instances = []  # Store plugin instances
        self.loaded_plugins = {}    # Track by name

    def register_plugin(self, plugin_name, plugin_instance):
        """Register a pre-created plugin instance"""
        self.plugin_instances.append(plugin_instance)
        self.loaded_plugins[plugin_name] = plugin_instance
        print(f"Loaded plugin: {plugin_name}")
    
    def initialize_all_plugins(self):
        """Call initialize method on all loaded plugins"""
        for plugin in self.plugin_instances:
            try:
                plugin.initialize()
            except AttributeError:
                # Plugin doesn't have initialize method, which is fine
                pass
    
    def handle_input_for_all_plugins(self, key):
        """Forward input to all plugins"""
        for plugin in self.plugin_instances:
            try:
                plugin.handle_input(key)
            except AttributeError:
                # Plugin doesn't have handle_input method, which is fine
                pass
    
    def update_all_plugins(self, dt):
        """Update all plugins"""
        for plugin in self.plugin_instances:
            try:
                plugin.update(dt)
            except AttributeError:
                # Plugin doesn't have update method, which is fine
                pass
    
    def find_plugin_by_capability(self, capability_attr):
        """Find a plugin that has a specific attribute/method"""
        for plugin in self.plugin_instances:
            if hasattr(plugin, capability_attr):
                return plugin
        return None


class PieceManager(PluginInterface):
    def __init__(self, game_engine):
        super().__init__(game_engine)
        self.pieces = {}
        self.piece_directory = "pieces/"
        
    def initialize(self):
        """Initialize by discovering and loading all pieces"""
        print("Piece Manager plugin initialized")
        self.discover_pieces()
        print(f"Loaded {len(self.pieces)} pieces")
    
    def discover_pieces(self):
        """Discover and load all pieces from the pieces directory"""
        if not os.path.exists(self.piece_directory):
            os.makedirs(self.piece_directory, exist_ok=True)
            print(f"Created piece directory: {self.piece_directory}")
            return
        
        # Discover all piece directories
        for piece_name in os.listdir(self.piece_directory):
            piece_path = os.path.join(self.piece_directory, piece_name)
            if os.path.isdir(piece_path):
                # Look for the piece file (should be named the same as the directory)
                # Special case: the fuzzpet piece is named "piece.txt" instead of "fuzzpet.txt"
                if piece_name == "fuzzpet":
                    piece_file = os.path.join(piece_path, "piece.txt")
                else:
                    piece_file = os.path.join(piece_path, f"{piece_name}.txt")
                
                if os.path.exists(piece_file):
                    self.pieces[piece_name] = self.load_piece_from_file(piece_file)
                    print(f"Loaded piece: {piece_name}")
                else:
                    # Create basic piece file if it doesn't exist
                    self.create_basic_piece_file(piece_path, piece_name)
                    if piece_name == "fuzzpet":
                        piece_file = os.path.join(piece_path, "piece.txt")
                    else:
                        piece_file = os.path.join(piece_path, f"{piece_name}.txt")
                    self.pieces[piece_name] = self.load_piece_from_file(piece_file)
                    print(f"Created basic piece: {piece_name}")
    
    def create_basic_piece_file(self, piece_path, piece_name):
        """Create a basic piece file if it doesn't exist"""
        piece_file = os.path.join(piece_path, f"{piece_name}.txt")
        if piece_name == "keyboard":
            with open(piece_file, 'w') as f:
                f.write("""session_id 12345
current_key_log_path pieces/keyboard/history.txt
logging_enabled true
keys_recorded 0
last_key_logged 
last_key_time 
last_key_code 0
status active
event_listeners [key_press]
methods [log_key, reset_session, get_session_log, enable_logging, disable_logging]
""")
        elif piece_name == "master_ledger":
            with open(piece_file, 'w') as f:
                f.write("""status active
last_entry_id 0
total_entries 0
event_listeners [system_event, method_call, state_change]
methods [log_event, read_log, search_log, clear_log]
""")
        elif piece_name == "mover":
            with open(piece_file, 'w') as f:
                f.write("""position_x 0
position_y 0
last_direction none
status active
event_listeners [key_w, key_a, key_s, key_d]
methods [move_up, move_down, move_left, move_right, get_position]
responses:
  key_w: Moving up
  key_a: Moving left  
  key_s: Moving down
  key_d: Moving right
""")
        elif piece_name == "command_processor":
            with open(piece_file, 'w') as f:
                f.write("""current_command 
command_buffer 
command_mode waiting
status active
event_listeners [key_press, command_entered]
methods [process_command, clear_buffer, execute_command]
responses:
  key_press: Key pressed
  command_entered: Command processed
""")
    
    def load_piece_from_file(self, file_path):
        """Load a piece's state from its text file"""
        piece_state = {}
        try:
            with open(file_path, 'r') as f:
                lines = f.readlines()
        
            # First pass: identify all sections and their boundaries
            sections = {}  # {line_index: section_name}
            for i, line in enumerate(lines):
                stripped = line.strip()
                if stripped.endswith(':') and not stripped.startswith('#') and not ' ' in stripped:
                    sections[i] = stripped[:-1].lower()
        
            # Second pass: parse content
            current_section = None
            section_start = -1
            
            for i, line in enumerate(lines):
                stripped = line.strip()
                
                if stripped.startswith('#') or not stripped:
                    continue
                    
                # Check if this line starts a new section
                if i in sections:
                    current_section = sections[i]
                    # Initialize the section dictionary if it doesn't exist
                    if current_section not in piece_state:
                        if current_section in ['state', 'responses']:
                            piece_state[current_section] = {}
                        elif current_section in ['methods', 'event_listeners']:
                            piece_state[current_section] = []
                    continue
                
                # Handle top-level methods/event_listeners (not in any section)
                if stripped.startswith('methods:') or stripped.startswith('event_listeners:'):
                    if stripped.startswith('methods:'):
                        value = stripped[len('methods:'):].strip()
                        items = [item.strip() for item in value.split(',') if item.strip()]
                        piece_state['methods'] = items
                    elif stripped.startswith('event_listeners:'):
                        value = stripped[len('event_listeners:'):].strip()
                        items = [item.strip() for item in value.split(',') if item.strip()]
                        piece_state['event_listeners'] = items
                # Handle content within sections
                elif current_section and ':' in stripped:
                    key, value = stripped.split(':', 1)
                    key = key.strip()
                    value = value.strip()
                    
                    if current_section in ['state', 'responses']:
                        piece_state[current_section][key] = value
                    elif current_section in ['methods', 'event_listeners']:
                        items = [item.strip() for item in value.split(',') if item.strip()]
                        if current_section not in piece_state:
                            piece_state[current_section] = []
                        piece_state[current_section].extend(items)
                elif ':' in stripped and current_section is None and not stripped.startswith('#'):
                    # Handle key:value pairs outside sections
                    parts = stripped.split(':', 1)
                    if len(parts) == 2:
                        key, value = parts[0].strip(), parts[1].strip()
                        # Try to convert numbers where appropriate
                        if key in ['keys_recorded', 'last_key_code', 'last_entry_id', 'total_entries', 'position_x', 'position_y', 'hunger', 'happiness', 'energy', 'level']:
                            try:
                                value = int(value)
                            except ValueError:
                                try:
                                    value = float(value)
                                except ValueError:
                                    pass  # Keep as string if not a valid number
                        piece_state[key] = value
        except Exception as e:
            print(f"Error loading piece from {file_path}: {e}")
        
        return piece_state
    
    def get_piece_state(self, piece_name):
        """Get the current state of a piece"""
        return self.pieces.get(piece_name, None)
    
    def update_piece_state(self, piece_name, new_state):
        """Update the state of a piece and save it to file"""
        self.pieces[piece_name] = new_state

        # Save to file - adjust file name for fuzzpet
        if piece_name == "fuzzpet":
            piece_path = os.path.join(self.piece_directory, piece_name, "piece.txt")
        else:
            piece_path = os.path.join(self.piece_directory, piece_name, f"{piece_name}.txt")
        
        try:
            with open(piece_path, 'w') as f:
                for key, value in new_state.items():
                    if key in ['state', 'responses'] and isinstance(value, dict):
                        # Handle section writing
                        f.write(f"{key}:\n")
                        for k, v in value.items():
                            f.write(f"  {k}: {v}\n")
                    elif key in ['methods', 'event_listeners'] and isinstance(value, list):
                        # Handle lists
                        f.write(f"{key}: {', '.join(value)}\n")
                    elif key == '# Piece' and isinstance(value, str):
                        # Handle special piece identifier
                        f.write(f"# {key}: {value}\n")
                    else:
                        # Handle regular key-value pairs
                        f.write(f"{key}: {value}\n")
        except Exception as e:
            print(f"Error saving piece {piece_name}: {e}")


class CommandProcessorPlugin(PluginInterface):
    def __init__(self, game_engine):
        super().__init__(game_engine)
        self.piece_manager = None
        self.command_processor_piece_name = 'command_processor'
        self.fuzzpet_piece_name = 'fuzzpet'
        self.master_ledger_file = 'pieces/master_ledger/master_ledger.txt'
        self.command_map = {
            '1': 'feed',
            '2': 'play', 
            '3': 'sleep',
            '4': 'status',
            '5': 'evolve',
            '6': 'quit'
        }
        self.command_buffer = ""
        
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
        print(f"\n[COMMAND] Executing: {command_name}")
        
        if self.piece_manager:
            # Get current fuzzpet state
            fuzzpet_state = self.piece_manager.get_piece_state(self.fuzzpet_piece_name)
            if fuzzpet_state:
                # Simulate command execution - in a full implementation, this would call real plugin methods
                if command_name == 'feed':
                    old_hunger = fuzzpet_state.get('state', {}).get('hunger', 30)
                    new_hunger = max(0, int(old_hunger) - 20)
                    fuzzpet_state['state']['hunger'] = new_hunger
                    print(f"[PET] Feeding FuzzPet! Hunger decreased from {old_hunger} to {new_hunger}")
                    
                    # Trigger fed event
                    self.trigger_event('fed')
                    
                elif command_name == 'play':
                    old_happiness = fuzzpet_state.get('state', {}).get('happiness', 55)
                    new_happiness = min(100, int(old_happiness) + 15)
                    fuzzpet_state['state']['happiness'] = new_happiness
                    print(f"[PET] Playing with FuzzPet! Happiness increased from {old_happiness} to {new_happiness}")
                    
                    # Trigger played event
                    self.trigger_event('played')
                    
                elif command_name == 'sleep':
                    old_energy = fuzzpet_state.get('state', {}).get('energy', 100)
                    new_energy = min(100, int(old_energy) + 20)
                    fuzzpet_state['state']['energy'] = new_energy
                    print(f"[PET] FuzzPet sleeping! Energy increased from {old_energy} to {new_energy}")
                    
                    # Trigger slept event
                    self.trigger_event('slept')
                    
                elif command_name == 'status':
                    state_vals = fuzzpet_state.get('state', {})
                    print(f"[PET STATUS] Name: {state_vals.get('name', 'FuzzPet')}, "
                          f"Hunger: {state_vals.get('hunger', 0)}, "
                          f"Happiness: {state_vals.get('happiness', 0)}, "
                          f"Energy: {state_vals.get('energy', 0)}, "
                          f"Level: {state_vals.get('level', 1)}")
                    
                elif command_name == 'evolve':
                    old_level = fuzzpet_state.get('state', {}).get('level', 1)
                    new_level = min(10, int(old_level) + 1)  # Cap at level 10
                    fuzzpet_state['state']['level'] = new_level
                    print(f"[PET] FuzzPet evolved! Level increased from {old_level} to {new_level}")
                    
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


class KeyboardPlugin(PluginInterface):
    def __init__(self, game_engine):
        super().__init__(game_engine)
        self.piece_manager = None
        self.command_processor = None
        self.keyboard_piece_name = 'keyboard'
        self.history_file_path = 'pieces/keyboard/history.txt'
        self.master_ledger_file = 'pieces/master_ledger/master_ledger.txt'
        self.last_processed_position = 0
        
    def initialize(self):
        """Initialize keyboard plugin by connecting to keyboard piece"""
        print("Keyboard plugin initialized")
        
        # Find the piece manager and command processor plugins
        for plugin in self.game_engine.plugin_manager.plugin_instances:
            if hasattr(plugin, 'get_piece_state'):
                self.piece_manager = plugin
            elif hasattr(plugin, 'handle_input') and plugin.__class__.__name__ == 'CommandProcessorPlugin':
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
            if keyboard_state:
                import time
                keyboard_state['session_id'] = int(time.time())  # Use timestamp as session ID
                keyboard_state['keys_recorded'] = 0
                keyboard_state['logging_enabled'] = True
                self.piece_manager.update_piece_state(self.keyboard_piece_name, keyboard_state)
    
    def log_key(self, key_char):
        """Log a key press to the history file and master ledger"""
        if not self.piece_manager:
            return False
            
        # Get current keyboard state
        keyboard_state = self.piece_manager.get_piece_state(self.keyboard_piece_name)
        if not keyboard_state or keyboard_state.get('logging_enabled', False) == False:
            return False
        
        try:
            # Write the key to the history file
            with open(self.history_file_path, 'a') as f:
                f.write(f"{ord(key_char)}\n")  # Write ASCII code like reference implementation
            
            # Update keyboard piece state
            keyboard_state['last_key_logged'] = key_char
            keyboard_state['last_key_code'] = ord(key_char)
            import time
            keyboard_state['last_key_time'] = time.strftime('%Y-%m-%d %H:%M:%S')
            keyboard_state['keys_recorded'] = int(keyboard_state.get('keys_recorded', 0)) + 1
            self.piece_manager.update_piece_state(self.keyboard_piece_name, keyboard_state)
            
            # Log to master ledger for audit trail
            timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
            log_entry = f"[{timestamp}] KeyEvent: {key_char} (code {ord(key_char)}) | Source: keyboard\n"
            with open(self.master_ledger_file, 'a') as ledger:
                ledger.write(log_entry)
                
            return True
        except Exception as e:
            print(f"Error logging key: {e}")
            return False
    
    def handle_input(self, key_char):
        """Handle direct input from the main loop"""
        if isinstance(key_char, str) and len(key_char) == 1:
            # Log the key for audit purposes
            self.log_key(key_char)
            
            # Forward command-related input to command processor
            # This makes number keys go to the command processor
            if self.command_processor and (key_char in '123456' or key_char.isdigit() or key_char in ['\n', '\r']):
                self.command_processor.handle_input(key_char)


class MoverPlugin(PluginInterface):
    def __init__(self, game_engine):
        super().__init__(game_engine)
        self.piece_manager = None
        self.mover_piece_name = 'mover'
        self.keyboard_plugin = None
        self.master_ledger_file = 'pieces/master_ledger/master_ledger.txt'
        
    def initialize(self):
        """Initialize mover plugin by connecting to required pieces"""
        print("Mover plugin initialized")
        
        # Find the piece manager plugin
        for plugin in self.game_engine.plugin_manager.plugin_instances:
            if hasattr(plugin, 'get_piece_state'):
                self.piece_manager = plugin
                break
            elif hasattr(plugin, 'log_key'):  # Find keyboard plugin
                self.keyboard_plugin = plugin
        
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
        """Handle input events - respond to WASD keys immediately"""
        if key_char.lower() in ['w', 'a', 's', 'd']:
            # Handle WASD movement immediately
            self.process_movement_key(key_char.lower())
    
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
        if key_char == 'w':
            mover_state['position_y'] = int(mover_state.get('position_y', 0)) - 1
            mover_state['last_direction'] = 'up'
            print(f"\n[MOVER] Moving up! New position: ({mover_state['position_x']}, {mover_state['position_y']})")
        elif key_char == 's':
            mover_state['position_y'] = int(mover_state.get('position_y', 0)) + 1
            mover_state['last_direction'] = 'down'
            print(f"\n[MOVER] Moving down! New position: ({mover_state['position_x']}, {mover_state['position_y']})")
        elif key_char == 'a':
            mover_state['position_x'] = int(mover_state.get('position_x', 0)) - 1
            mover_state['last_direction'] = 'left'
            print(f"\n[MOVER] Moving left! New position: ({mover_state['position_x']}, {mover_state['position_y']})")
        elif key_char == 'd':
            mover_state['position_x'] = int(mover_state.get('position_x', 0)) + 1
            mover_state['last_direction'] = 'right'
            print(f"\n[MOVER] Moving right! New position: ({mover_state['position_x']}, {mover_state['position_y']})")
        
        # Update piece state
        self.piece_manager.update_piece_state(self.mover_piece_name, mover_state)
        
        # Log to master ledger for audit trail
        import time
        timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
        log_entry = f"[{timestamp}] MovementEvent: {mover_state['last_direction']} | From: ({old_x},{old_y}) To: ({mover_state['position_x']},{mover_state['position_y']}) | Target: mover\n"
        with open(self.master_ledger_file, 'a') as ledger:
            ledger.write(log_entry)
        
        # Create an event that could be listened to by other pieces
        event_name = f"key_{key_char}"
        self.trigger_event(event_name, 'mover')
    
    def trigger_event(self, event_name, piece_id):
        """Simulate triggering an event (future event manager would handle this)"""
        # For now, just log to master ledger
        import time
        timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
        log_entry = f"[{timestamp}] EventFire: {event_name} on {piece_id} | Trigger: movement_key_press\n"
        with open(self.master_ledger_file, 'a') as ledger:
            ledger.write(log_entry)
    
    def update(self, dt):
        """Check for recent key inputs from keyboard history"""
        # This method would normally check the keyboard history file for new inputs
        # But since we handle input directly in handle_input, this is just for completeness
        pass


class TPMEngine:
    """Main engine that coordinates all plugins"""
    
    def __init__(self):
        self.plugin_manager = PluginManager()
        self.running = True
        
        # Initialize the pieces directory
        self.pieces_dir = Path("pieces")
        self.pieces_dir.mkdir(parents=True, exist_ok=True)
        
        # Create core pieces
        self._initialize_core_pieces()
        
        # Load and register plugins
        self.piece_manager = PieceManager(self)
        self.keyboard_plugin = KeyboardPlugin(self)
        self.mover_plugin = MoverPlugin(self)
        self.command_processor = CommandProcessorPlugin(self)
        
        self.plugin_manager.register_plugin('core.piece_manager', self.piece_manager)
        self.plugin_manager.register_plugin('core.keyboard', self.keyboard_plugin)
        self.plugin_manager.register_plugin('mover', self.mover_plugin)
        self.plugin_manager.register_plugin('command_processor', self.command_processor)
        
        # Initialize all plugins
        self.plugin_manager.initialize_all_plugins()
        
        print("\n=== TPM Keyboard Foundation Active ===")
        print("Press WASD keys for immediate movement feedback")
        print("Press 1-5 for pet commands (feed, play, sleep, status, evolve)")
        print("Press other keys to see them logged")
        print("Check pieces/master_ledger/master_ledger.txt for audit trail")
        print("Press Ctrl+C to quit")
        print("="*50)
    
    def _initialize_core_pieces(self):
        """Initialize core pieces if they don't exist"""
        # Create keyboard piece directory and file
        keyboard_dir = self.pieces_dir / "keyboard"
        keyboard_dir.mkdir(exist_ok=True)
        
        keyboard_file = keyboard_dir / "keyboard.txt"
        if not keyboard_file.exists():
            with open(keyboard_file, 'w') as f:
                f.write("""session_id 12345
current_key_log_path pieces/keyboard/history.txt
logging_enabled true
keys_recorded 0
last_key_logged 
last_key_time 
last_key_code 0
status active
event_listeners [key_press]
methods [log_key, reset_session, get_session_log, enable_logging, disable_logging]
""")
        
        # Create master ledger piece directory and file
        master_ledger_dir = self.pieces_dir / "master_ledger"
        master_ledger_dir.mkdir(exist_ok=True)
        
        master_ledger_file = master_ledger_dir / "master_ledger.txt"
        if not master_ledger_file.exists():
            with open(master_ledger_file, 'w') as f:
                f.write("# Master Ledger - Central Audit Trail\n")
        
        # Create mover piece directory and file
        mover_dir = self.pieces_dir / "mover"
        mover_dir.mkdir(exist_ok=True)
        
        mover_file = mover_dir / "mover.txt"
        if not mover_file.exists():
            with open(mover_file, 'w') as f:
                f.write("""position_x 0
position_y 0
last_direction none
status active
event_listeners [key_w, key_a, key_s, key_d]
methods [move_up, move_down, move_left, move_right, get_position]
responses:
  key_w: Moving up
  key_a: Moving left  
  key_s: Moving down
  key_d: Moving right
""")
        
        # Create command_processor piece directory and file
        command_processor_dir = self.pieces_dir / "command_processor"
        command_processor_dir.mkdir(exist_ok=True)
        
        command_processor_file = command_processor_dir / "command_processor.txt"
        if not command_processor_file.exists():
            with open(command_processor_file, 'w') as f:
                f.write("""current_command 
command_buffer 
command_mode waiting
status active
event_listeners [key_press, command_entered]
methods [process_command, clear_buffer, execute_command]
responses:
  key_press: Key pressed
  command_entered: Command processed
""")
        
        # Create fuzzpet piece directory and file (from existing data)
        fuzzpet_dir = self.pieces_dir / "fuzzpet"
        fuzzpet_dir.mkdir(exist_ok=True)
        
        fuzzpet_file = fuzzpet_dir / "piece.txt"
        if not fuzzpet_file.exists():
            with open(fuzzpet_file, 'w') as f:
                f.write("""# Piece: fuzzpet
state:
  name: Fuzzball
  hunger: 30
  happiness: 55
  energy: 100
  level: 1
methods: feed, play, sleep, status, evolve
event_listeners: fed, played, slept, command_received
responses:
  fed: Yum yum~ Feeling so happy! *tail wags wildly*
  played: Wheee~ Best playtime ever! *bounces around joyfully*
  slept: Zzz... all cozy now~ *yawns cutely* Feeling recharged!
  command_received: *ears perk up* Ooh, you said something? *tilts head curiously*
  evolve: *glows brightly* Level up! I'm getting stronger~!
  default: *confused tilt* ...huh?
""")
    
    def run(self):
        """Run the main loop with immediate keyboard input"""
        import sys
        import tty
        import termios
        import select
        
        # Setup terminal for raw input
        old_settings = None
        try:
            old_settings = termios.tcgetattr(sys.stdin)
            tty.setcbreak(sys.stdin.fileno())
        except:
            print("Warning: Could not setup raw input mode")
            return
        
        try:
            print("\nTPM Foundation running... press WASD for immediate response\n")
            
            while self.running:
                # Check for input without blocking
                if select.select([sys.stdin], [], [], 0) == ([sys.stdin], [], []):
                    try:
                        ch = sys.stdin.read(1)
                        # Send input to all plugins for processing
                        self.plugin_manager.handle_input_for_all_plugins(ch)
                        
                        # Echo the character (non-blocking)
                        if ch == '\x03':  # Ctrl+C
                            print("\nQuitting...")
                            break
                        elif ch == '\x7f':  # Backspace
                            print("^H", end="", flush=True)
                        elif ch == '\n' or ch == '\r':  # Enter
                            print("^M", end="", flush=True)
                        elif ch == '\t':  # Tab
                            print("^I", end="", flush=True)
                        else:
                            # Echo non-control characters
                            if ord(ch) >= 32:  # Printable characters
                                print(ch, end="", flush=True)
                                
                    except Exception as e:
                        print(f"Input error: {e}")
                        continue
                
                # Small delay to prevent excessive CPU usage
                time.sleep(0.01)
        
        except KeyboardInterrupt:
            print("\nInterrupted by user")
        finally:
            # Restore terminal
            if old_settings:
                try:
                    termios.tcsetattr(sys.stdin, termios.TCSADRAIN, old_settings)
                except:
                    pass
            print("\nTPM Foundation shutdown complete")


def main():
    """Main entry point"""
    print("Starting TPM Foundation with immediate keyboard input...")
    engine = TPMEngine()
    engine.run()


if __name__ == "__main__":
    main()