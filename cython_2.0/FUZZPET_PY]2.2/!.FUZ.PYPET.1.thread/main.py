import os
import sys
import time
from collections import defaultdict

sys.setrecursionlimit(100)

class PieceManager:
    def __init__(self, pieces_dir='pieces'):
        self.pieces_dir = pieces_dir
        self.pieces = {}
        self.piece_paths = {}  # Map piece_id to its directory path
        self.load_all()

        # Debug: Print loaded pieces and their contents
        print("\n[DEBUG] Loaded pieces and their listeners:")
        for pid, d in self.pieces.items():
            print(f"  {pid}:")
            print(f"    listeners   = {d.get('event_listeners', [])}")
            print(f"    methods     = {d.get('methods', [])}")
            print(f"    responses   = {d.get('responses', {})}")
            print(f"    state keys  = {list(d.get('state', {}).keys())}")

    def load_all(self):
        for piece_dir_name in os.listdir(self.pieces_dir):
            piece_path = os.path.join(self.pieces_dir, piece_dir_name)
            if os.path.isdir(piece_path):
                piece_txt_path = os.path.join(piece_path, 'piece.txt')
                if os.path.exists(piece_txt_path):
                    self.piece_paths[piece_dir_name] = piece_path
                    self.pieces[piece_dir_name] = self.parse_piece(piece_txt_path)

    def parse_piece(self, filepath):
        data = {
            'state': {},
            'methods': [],
            'event_listeners': [],
            'responses': {}
        }
        with open(filepath, 'r') as f:
            lines = f.readlines()
        
        # First pass: identify all sections and their boundaries
        sections = {}  # {line_index: section_name}
        for i, line in enumerate(lines):
            stripped = line.strip()
            if stripped.endswith(':') and not stripped.startswith('#'):
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
                section_start = i
                continue
            
            # Handle top-level methods/event_listeners (not in any section)
            if stripped.startswith('methods:') or stripped.startswith('event_listeners:'):
                if stripped.startswith('methods:'):
                    value = stripped[len('methods:'):].strip()
                    items = [item.strip() for item in value.split(',') if item.strip()]
                    data['methods'].extend(items)
                elif stripped.startswith('event_listeners:'):
                    value = stripped[len('event_listeners:'):].strip()
                    items = [item.strip() for item in value.split(',') if item.strip()]
                    data['event_listeners'].extend(items)
            # Handle content within sections
            elif current_section and ':' in stripped:
                key, value = stripped.split(':', 1)
                key = key.strip()
                value = value.strip()
                
                if current_section in ['state', 'responses']:
                    data[current_section][key] = value
                elif current_section in ['methods', 'event_listeners']:
                    items = [item.strip() for item in value.split(',') if item.strip()]
                    data[current_section].extend(items)
        
        return data

    def get_state(self, piece_id, key=None):
        state = self.pieces.get(piece_id, {}).get('state', {})
        if key:
            value = state.get(key)
            if value is not None:
                try:
                    if '.' in value:
                        return float(value)
                    return int(value)
                except ValueError:
                    return value
            return None
        return state

    def set_state(self, piece_id, key, value):
        if piece_id not in self.pieces:
            return
        self.pieces[piece_id]['state'][key] = str(value)
        self.save_piece(piece_id)
        
        # Log the state change to the master ledger
        timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
        log_entry = f"[{timestamp}] StateChange: {piece_id} {key} {value} | Trigger: set_state\n"
        with open('pieces/master_ledger/master_ledger.txt', 'a') as ledger:
            ledger.write(log_entry)
        
        # Fire state changed event
        event_manager.fire('state_change', piece_id)

    def save_piece(self, piece_id):
        if piece_id not in self.piece_paths:
            return
        piece_path = self.piece_paths[piece_id]
        filepath = os.path.join(piece_path, 'piece.txt')
        data = self.pieces[piece_id]
        
        with open(filepath, 'w') as f:
            f.write(f"# Piece: {piece_id}\n")
            f.write("state:\n")
            for k, v in data['state'].items():
                f.write(f"  {k}: {v}\n")
            if data['methods']:
                f.write("methods: " + ', '.join(data['methods']) + "\n")
            if data['event_listeners']:
                f.write("event_listeners: " + ', '.join(data['event_listeners']) + "\n")
            if data['responses']:
                f.write("responses:\n")
                for k, v in data['responses'].items():
                    f.write(f"  {k}: {v}\n")

class MethodsManager:
    def __init__(self):
        self.methods = {}

    def register(self, method_name, func):
        self.methods[method_name] = func

    def call(self, method_name, piece_id, *args, **kwargs):
        if method_name in self.methods:
            return self.methods[method_name](piece_id, *args, **kwargs)
        print(f"[ERROR] Method '{method_name}' not found for {piece_id}")
        return None

class EventManager:
    def __init__(self):
        self.listeners = defaultdict(list)

    def register_all_listeners(self):
        self.listeners.clear()
        print("\n[DEBUG] Registering listeners...")
        for piece_id, data in piece_manager.pieces.items():
            for event_name in data.get('event_listeners', []):
                respond_method = f"respond_to_{event_name}"
                if respond_method in methods_manager.methods:
                    self.listeners[event_name].append((piece_id, respond_method))
                    print(f"[LISTENER REGISTERED] {piece_id} → {respond_method} for '{event_name}'")
                else:
                    print(f"[LISTENER SKIPPED] {piece_id} wants '{event_name}' but no method '{respond_method}' exists")

    def fire(self, event_name, piece_id=None):
        print(f"[EVENT] Fired: {event_name} (target: {piece_id or 'global'})")
        
        # Log the event to the master ledger
        timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
        log_entry = f"[{timestamp}] EventFire: {event_name} on {piece_id or 'global'} | Trigger: fire_event\n"
        with open('pieces/master_ledger/master_ledger.txt', 'a') as ledger:
            ledger.write(log_entry)
        
        matching_listeners = self.listeners.get(event_name, [])
        if not matching_listeners:
            print(f"[EVENT] No listeners found for '{event_name}'")
            return
        for listener_piece_id, respond_method_name in matching_listeners:
            if listener_piece_id in piece_manager.pieces:
                print(f"  → Dispatching to {listener_piece_id} via {respond_method_name}")
                delta = methods_manager.call(respond_method_name, listener_piece_id)
                if delta:
                    for k, v in delta.items():
                        piece_manager.set_state(listener_piece_id, k, v)
            else:
                print(f"  → Skipped missing piece: {listener_piece_id}")

class SingleThreadLedgerProcessor:
    def __init__(self):
        self.last_position = 0
        # Initialize to end of file to skip historical entries on first start
        try:
            with open('pieces/master_ledger/master_ledger.txt', 'a') as ledger:
                ledger.seek(0, 2)  # Seek to end of file
                self.last_position = ledger.tell()
        except FileNotFoundError:
            # File doesn't exist yet, that's ok
            pass

    def process_new_ledger_entries(self):
        """Process any new entries in the master ledger file"""
        try:
            with open('pieces/master_ledger/master_ledger.txt', 'r') as ledger:
                ledger.seek(self.last_position)
                new_lines = ledger.readlines()
                self.last_position = ledger.tell()
                
            for line in new_lines:
                line = line.strip()
                if not line or line.startswith('#'):
                    continue
                    
                # Parse the ledger entry and process accordingly
                if 'MethodCall:' in line:
                    self._handle_method_call(line)
                elif 'EventFire:' in line:
                    self._handle_event_fire(line)
                elif 'StateChange:' in line:
                    self._handle_state_change(line)
        except FileNotFoundError:
            # Wait for the ledger to be created
            pass

    def _handle_method_call(self, line):
        """Handle method call entries in the master ledger"""
        print(f"[LEDGER PROCESSOR] Processing method call: {line}")
        # Extract method and target from the line
        # Format: [TIMESTAMP] MethodCall: method_name on piece_id | Caller: source
        try:
            parts = line.split('MethodCall: ')[1].split(' |')[0]
            method_and_target = parts.split(' on ')
            if len(method_and_target) >= 2:
                method_name = method_and_target[0].strip()
                piece_id = method_and_target[1].strip()
                
                # Determine the event that should fire after this method
                # This maps method names to event names
                method_to_event_map = {
                    'feed': 'fed',
                    'play': 'played', 
                    'sleep': 'slept',
                    'evolve': 'evolve'
                }
                
                # Determine arguments based on method name
                method_args = []
                if method_name == 'feed':
                    method_args = [20]  # amount=20
                elif method_name == 'play':
                    method_args = [30]  # intensity=30
                elif method_name == 'sleep':
                    method_args = [8]   # hours=8
                # For status and evolve, no additional args needed
                
                # Call the method with appropriate arguments
                delta = methods_manager.call(method_name, piece_id, *method_args)
                if delta:
                    for k, v in delta.items():
                        piece_manager.set_state(piece_id, k, v)
                
                # Fire appropriate event after method execution
                if method_name in method_to_event_map:
                    event_name = method_to_event_map[method_name]
                    event_manager.fire(event_name, piece_id)
        except Exception as e:
            print(f"[LEDGER PROCESSOR] Error processing method call: {e}")

    def _handle_event_fire(self, line):
        """Handle event fire entries in the master ledger"""
        print(f"[LEDGER PROCESSOR] Processing event fire: {line}")
        # Extract event and target from the line
        try:
            parts = line.split('EventFire: ')[1].split(' |')[0]
            event_and_target = parts.split(' on ')
            if len(event_and_target) >= 2:
                event_name = event_and_target[0].strip()
                piece_id = event_and_target[1].strip()
                
                # Fire the event
                event_manager.fire(event_name, piece_id)
        except Exception as e:
            print(f"[LEDGER PROCESSOR] Error processing event fire: {e}")

    def _handle_state_change(self, line):
        """Handle state change entries in the master ledger"""
        print(f"[LEDGER PROCESSOR] Processing state change: {line}")
        # State changes are already handled by piece_manager.set_state, so we log them


# Global instances
piece_manager = PieceManager()
methods_manager = MethodsManager()
event_manager = EventManager()
ledger_processor = SingleThreadLedgerProcessor()

# Load plugins
print("\nLoading plugins...")
with open('plugin.config', 'r') as f:
    for line in f:
        plugin_path = line.strip()
        if plugin_path:
            print(f"  → Loading {plugin_path}")
            import importlib.util
            spec = importlib.util.spec_from_file_location("plugin", plugin_path)
            plugin = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(plugin)
            for name, func in plugin.__dict__.items():
                if callable(func) and not name.startswith('_'):
                    methods_manager.register(name, func)
            
            # Set the managers in the plugin to avoid import issues
            if hasattr(plugin, 'set_managers'):
                plugin.set_managers(piece_manager, methods_manager, event_manager)

# Debug registered methods
print("\n[DEBUG] Registered methods:")
for name in sorted(methods_manager.methods.keys()):
    print(f"  - {name}")

# Register listeners
event_manager.register_all_listeners()

print("\n=== TPM Virtual Pet – FuzzPet ===")
print("Commands: feed, play, sleep, status, evolve")
print("Shortcuts: 1=feed, 2=play, 3=sleep, 4=status, 5=evolve")
print("Pet reacts with messages & state updates!\n")

# Define command mapping with numeric shortcuts
command_map = {
    '1': 'feed',
    '2': 'play', 
    '3': 'sleep',
    '4': 'status',
    '5': 'evolve'
}

action_map = {
    'feed':   ('feed', 'fed', 20),
    'play':   ('play', 'played', 30),
    'sleep':  ('sleep', 'slept', 8),
    'evolve': ('evolve', 'evolve', None),
    'status': ('status', None, None),
}

while True:
    cmd = input("> ").strip().lower()
    
    # Convert numeric shortcut to command name
    if cmd in command_map:
        cmd = command_map[cmd]
    
    if not cmd:
        continue

    if cmd in action_map:  # Only process valid commands
        method_name, event_name, *args = action_map[cmd]
        
        # Log the user command to the master ledger as a MethodCall
        timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
        log_entry = f"[{timestamp}] MethodCall: {method_name} on fuzzpet | Caller: user_input\n"
        with open('pieces/master_ledger/master_ledger.txt', 'a') as ledger:
            ledger.write(log_entry)
            ledger.flush()  # Ensure data is written to disk
            import os
            os.fsync(ledger.fileno())  # Force sync to disk

        # Fire command_received event immediately
        event_manager.fire('command_received', 'fuzzpet')
        
        # Process the newly added ledger entry immediately
        ledger_processor.process_new_ledger_entries()
    else:
        if cmd != 'quit':  # Don't show message for quit command
            print("[Pet] *tilts head* ...huh?")