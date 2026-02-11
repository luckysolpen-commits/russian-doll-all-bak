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
                piece_file = os.path.join(piece_path, f"{piece_name}.txt")
                if os.path.exists(piece_file):
                    self.pieces[piece_name] = self.load_piece_from_file(piece_file)
                    print(f"Loaded piece: {piece_name}")
                else:
                    # Create basic piece file if it doesn't exist
                    self.create_basic_piece_file(piece_path, piece_name)
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
    
    def load_piece_from_file(self, file_path):
        """Load a piece's state from its text file"""
        piece_state = {}
        try:
            with open(file_path, 'r') as f:
                for line in f:
                    line = line.strip()
                    if line and not line.startswith('#'):
                        parts = line.split(' ', 1)
                        if len(parts) >= 2:
                            key = parts[0]
                            value = parts[1]
                            
                            # Try to convert numbers where appropriate
                            if key in ['keys_recorded', 'last_key_code', 'last_entry_id', 'total_entries', 'position_x', 'position_y']:
                                try:
                                    value = int(value)
                                except ValueError:
                                    pass  # Keep as string if not a valid int
                            elif key.startswith('option_'):
                                # Options are strings
                                pass
                            elif isinstance(value, str) and value.lower() in ('true', 'false'):
                                value = value.lower() == 'true'
                            
                            piece_state[key] = value
                        elif len(parts) == 1:
                            # Handle single keywords
                            piece_state[parts[0]] = True
        except Exception as e:
            print(f"Error loading piece from {file_path}: {e}")
        
        return piece_state
    
    def get_piece_state(self, piece_name):
        """Get the current state of a piece"""
        return self.pieces.get(piece_name, None)
    
    def update_piece_state(self, piece_name, new_state):
        """Update the state of a piece and save it to file"""
        self.pieces[piece_name] = new_state

        # Save to file
        piece_path = os.path.join(self.piece_directory, piece_name, f"{piece_name}.txt")
        try:
            with open(piece_path, 'w') as f:
                for key, value in new_state.items():
                    # Convert certain values back to string format for saving
                    if key in ['keys_recorded', 'last_key_code', 'last_entry_id', 'total_entries', 'position_x', 'position_y']:
                        f.write(f"{key} {value}\n")
                    else:
                        f.write(f"{key} {value}\n")
        except Exception as e:
            print(f"Error saving piece {piece_name}: {e}")


class KeyboardPlugin(PluginInterface):
    def __init__(self, game_engine):
        super().__init__(game_engine)
        self.piece_manager = None
        self.keyboard_piece_name = 'keyboard'
        self.history_file_path = 'pieces/keyboard/history.txt'
        self.master_ledger_file = 'pieces/master_ledger/master_ledger.txt'
        self.last_processed_position = 0
        
    def initialize(self):
        """Initialize keyboard plugin by connecting to keyboard piece"""
        print("Keyboard plugin initialized")
        
        # Find the piece manager plugin
        for plugin in self.game_engine.plugin_manager.plugin_instances:
            if hasattr(plugin, 'get_piece_state'):
                self.piece_manager = plugin
                break
        
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
            # Only log single character keys
            self.log_key(key_char)


class MoverPlugin(PluginInterface):
    def __init__(self, game_engine):
        super().__init__(game_engine)
        self.piece_manager = None
        self.mover_piece_name = 'mover'
        self.keyboard_plugin = None
        
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
        with open('pieces/master_ledger/master_ledger.txt', 'a') as ledger:
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
        with open('pieces/master_ledger/master_ledger.txt', 'a') as ledger:
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
        
        self.plugin_manager.register_plugin('core.piece_manager', self.piece_manager)
        self.plugin_manager.register_plugin('core.keyboard', self.keyboard_plugin)
        self.plugin_manager.register_plugin('mover', self.mover_plugin)
        
        # Initialize all plugins
        self.plugin_manager.initialize_all_plugins()
        
        print("\n=== TPM Keyboard Foundation Active ===")
        print("Press WASD keys for immediate movement feedback")
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