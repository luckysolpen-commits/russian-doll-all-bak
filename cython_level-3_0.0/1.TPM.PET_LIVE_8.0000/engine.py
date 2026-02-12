"""
TPM Foundation - Keyboard Input with Master Ledger (Refactored)
Demonstrates proper plugin architecture following TPM principles
"""

import sys
import tty
import termios
import select
import time
import os
from pathlib import Path


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


class MasterLedgerMonitor:
    """Monitors the master ledger for new entries and processes them according to TPM principles"""
    def __init__(self, engine):
        self.engine = engine
        self.file_path = 'pieces/master_ledger/ledger.txt'
        self.last_position = 0  # Will start reading from beginning, but the file will be fresh with only header
        self.monitoring = False
        
    def start_monitoring(self):
        """Start monitoring the master ledger file for new entries"""
        self.monitoring = True
        
    def process_new_entries(self):
        """Process any new entries in the master ledger"""
        if not self.monitoring:
            return
            
        try:
            with open(self.file_path, 'r') as ledger:
                ledger.seek(self.last_position)
                new_lines = ledger.readlines()
                self.last_position = ledger.tell()
                
            for line in new_lines:
                line = line.strip()
                if not line or line.startswith('#'):
                    continue
                self.parse_and_execute_line(line)
        except FileNotFoundError:
            pass  # Wait for file to be created

    def parse_and_execute_line(self, line):
        """Parse a ledger entry and execute appropriate action"""
        if 'MethodCall:' in line:
            self._handle_method_call(line)
        elif 'EventFire:' in line:
            self._handle_event_fire(line)
        elif 'KeyEvent:' in line:
            self._handle_key_event(line)
        elif 'MovementEvent:' in line:
            # Handle movement events to update display
            for plugin in self.engine.plugin_manager.plugin_instances:
                if hasattr(plugin, 'update_display'):
                    plugin.update_display()
    
    def _handle_method_call(self, line):
        """Process a MethodCall entry from the master ledger"""
        try:
            # Parse: [TIMESTAMP] MethodCall: method_name on piece_id | Caller: source
            parts = line.split('MethodCall: ')[1].split(' |')[0]
            method_and_target = parts.split(' on ')
            if len(method_and_target) >= 2:
                method_name = method_and_target[0].strip()
                piece_id = method_and_target[1].strip()
                
                # Find methods manager to call the method (would be the piece manager in real TPM)
                # For now, we'll simulate calling the method by finding the right plugin
                for plugin in self.engine.plugin_manager.plugin_instances:
                    if hasattr(plugin, 'execute_command') and piece_id == 'fuzzpet':
                        # For fuzzpet commands, use command processor
                        plugin.execute_command(method_name)
                        # Update display after command execution
                        for display_plugin in self.engine.plugin_manager.plugin_instances:
                            if hasattr(display_plugin, 'update_display'):
                                display_plugin.update_display()
                    elif hasattr(plugin, 'process_movement_key') and piece_id == 'mover':
                        # For movement, find the key from the event
                        if method_name.startswith('key_') and len(method_name) == 5:  # e.g., key_w
                            key = method_name[4]  # Get the letter after 'key_'
                            plugin.process_movement_key(key)  # This handles movement and logs appropriately
        except (IndexError, AttributeError):
            pass

    def _handle_event_fire(self, line):
        """Process an EventFire entry from the master ledger"""
        try:
            # Parse: [TIMESTAMP] EventFire: event_name on piece_id | Trigger: source
            parts = line.split('EventFire: ')[1].split(' |')[0]
            event_and_target = parts.split(' on ')
            if len(event_and_target) >= 2:
                event_name = event_and_target[0].strip()
                piece_id = event_and_target[1].strip()
                
                # Handle specific events
                if event_name.startswith('key_') and piece_id == 'mover':
                    # WASD key events for movement
                    if len(event_name) == 5:  # e.g., key_w
                        key = event_name[4]  # Get the letter after 'key_'
                        for plugin in self.engine.plugin_manager.plugin_instances:
                            if hasattr(plugin, 'process_movement_key'):
                                plugin.process_movement_key(key)  # This handles the movement and logs appropriately
        except (IndexError, AttributeError):
            pass
    
    def _handle_key_event(self, line):
        """Process a KeyEvent entry from the master ledger (for logging mainly)"""
        pass  # KeyEvent is already logged, just processed for completeness


class TPMEngine:
    """Main engine that coordinates all plugins"""
    
    def __init__(self):
        self.plugin_manager = PluginManager()
        self.running = True
        
        # Initialize the pieces directory
        self.pieces_dir = Path("pieces")
        self.pieces_dir.mkdir(parents=True, exist_ok=True)
        
        # CLEAR ALL RELEVANT FILES FOR FRESH START
        self._clear_all_relevant_files()
        
        # Initialize core pieces if they don't exist
        self._initialize_core_pieces()
        
        # Load and register plugins dynamically
        self._load_plugins()
        
        # Initialize all plugins
        self.plugin_manager.initialize_all_plugins()
        
        # Initialize the master ledger monitor
        self.master_ledger_monitor = MasterLedgerMonitor(self)
        self.master_ledger_monitor.start_monitoring()  # START MONITORING
        
        print("\n=== TPM Keyboard Foundation Active (Refactored) ===")
        print("Check pieces/master_ledger/master_ledger.txt for audit trail")
        print("Press Ctrl+C to quit")
        print("="*50)
        
        # Show initial map with menu options - this will show the condensed dynamic menu
        print("\033[2J\033[H", end="")  # ANSI codes to clear screen and move cursor to top-left
        # Find and call the display plugin's update method initially
        display_plugin = self.plugin_manager.find_plugin_by_capability('update_display')
        if display_plugin and hasattr(display_plugin, 'update_display'):
            display_plugin.update_display()
    
    def _clear_all_relevant_files(self):
        """Clear all relevant files for a completely fresh start"""
        import os
        
        # Clear keyboard history
        keyboard_history = self.pieces_dir / "keyboard" / "history.txt"
        if keyboard_history.exists():
            keyboard_history.unlink()  # Remove the file
        # Recreate with header
        keyboard_history.parent.mkdir(parents=True, exist_ok=True)
        with open(keyboard_history, 'w') as f:
            f.write("# Keyboard session started\n")
            
        # Clear master ledger
        master_ledger = self.pieces_dir / "master_ledger" / "ledger.txt"
        if master_ledger.exists():
            master_ledger.unlink()  # Remove the file
        # Recreate with header
        master_ledger.parent.mkdir(parents=True, exist_ok=True)
        with open(master_ledger, 'w') as f:
            f.write("# Master Ledger - Central Audit Trail\n")
            
        # Clear display frame history
        frame_history = self.pieces_dir / "display" / "frame_history.txt"
        if frame_history.exists():
            frame_history.unlink()  # Remove the file
        # Recreate with header
        frame_history.parent.mkdir(parents=True, exist_ok=True)
        with open(frame_history, 'w') as f:
            f.write("# Frame History - Latest frame at end\n")
            
        # Clear current frame
        current_frame = self.pieces_dir / "display" / "current_frame.txt"
        if current_frame.exists():
            current_frame.unlink()  # Remove the file
        # Recreate empty
        with open(current_frame, 'w') as f:
            f.write("")
    
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
        
        # Create the ledger log file
        ledger_file = master_ledger_dir / "ledger.txt"
        if not ledger_file.exists():
            with open(ledger_file, 'w') as f:
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
  pos_x: 5
  pos_y: 5
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
        
        # Create world piece directory and file
        world_dir = self.pieces_dir / "world"
        world_dir.mkdir(exist_ok=True)
        
        world_file = world_dir / "world.txt"
        if not world_file.exists():
            with open(world_file, 'w') as f:
                f.write("""status active
event_listeners [map_update]
methods [render_map, update_view]
""")
        
        # Create a simpler 10x20 map file
        map_file_path = self.pieces_dir / "world" / "map.txt"
        if not map_file_path.exists():
            with open(map_file_path, 'w') as f:
                f.write("""##########
#        #
#   @    #
#        #
#        #
#        #
#        #
#        #
#        #
##########
""")
        
        # Create display piece directory and files
        display_dir = self.pieces_dir / "display"
        display_dir.mkdir(exist_ok=True)
        
        display_file = display_dir / "display.txt"
        if not display_file.exists():
            with open(display_file, 'w') as f:
                f.write("""status active
current_frame 
frame_count 0
event_listeners [frame_requested, display_update]
methods [render_frame, clear_frame, save_frame]
""")

        # Create frame history and current frame files
        frame_history_path = self.pieces_dir / "display" / "frame_history.txt"
        if not frame_history_path.exists():
            with open(frame_history_path, 'w') as f:
                f.write("# Frame History - Latest frame at end\n")

        current_frame_path = self.pieces_dir / "display" / "current_frame.txt"
        if not current_frame_path.exists():
            with open(current_frame_path, 'w') as f:
                f.write("")
    
    def _load_plugins(self):
        """Dynamically load plugins from the plugins/ directory"""
        import importlib.util
        import os
        
        # Load plugins from core directory
        core_plugins_path = Path("plugins/core")
        for plugin_file in core_plugins_path.glob("*.py"):
            if plugin_file.name != "__init__.py":  # Skip __init__.py files
                plugin_name = plugin_file.stem
                try:
                    spec = importlib.util.spec_from_file_location(plugin_name, plugin_file)
                    plugin_module = importlib.util.module_from_spec(spec)
                    spec.loader.exec_module(plugin_module)
                    
                    # Look for a Plugin class in the module
                    if hasattr(plugin_module, 'Plugin'):
                        plugin_class = plugin_module.Plugin
                        plugin_instance = plugin_class(self)
                        self.plugin_manager.register_plugin(f'core.{plugin_name}', plugin_instance)
                    else:
                        print(f"Warning: No Plugin class found in {plugin_file}")
                        
                except Exception as e:
                    print(f"Error loading plugin {plugin_file}: {e}")
    
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
            print("Warning: Could not setup raw input mode. This may affect keyboard input handling.")
            # Continue anyway since we can still run in non-raw mode
        
        try:
            print("\nTPM Foundation running... press WASD for immediate response\n")
            
            while self.running:
                # Process master ledger entries
                self.master_ledger_monitor.process_new_entries()
                
                # Update all plugins periodically
                current_time = time.time()
                dt = 0.01  # Time delta for update
                
                for plugin in self.plugin_manager.plugin_instances:
                    try:
                        plugin.update(dt)
                    except AttributeError:
                        # Plugin doesn't have update method, which is fine
                        pass
                
                # Check for input without blocking
                if select.select([sys.stdin], [], [], 0.01) == ([sys.stdin], [], []):
                    try:
                        ch = sys.stdin.read(1)
                        # Send input to all plugins for processing
                        self.plugin_manager.handle_input_for_all_plugins(ch)
                        
                        # Echo the character (non-blocking)
                        if ch == '\x03':  # Ctrl+C
                            print("\nQuitting...")
                            self.running = False
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
    print("Starting TPM Foundation with immediate keyboard input (Refactored)...")
    engine = TPMEngine()
    engine.run()


if __name__ == "__main__":
    main()