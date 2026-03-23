"""
Desktop Engine for PyOS - Virtual Operating System (TPM Version)
Implements the True Piece Method architecture with pieces, master ledger, and plugins
"""

import sys
import tty
import termios
import select
import time
import os
from pathlib import Path
from PySide6.QtWidgets import QApplication
from plugin_interface import PluginInterface


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
                
                # Find appropriate plugin to handle the method call
                for plugin in self.engine.plugin_manager.plugin_instances:
                    if hasattr(plugin, 'execute_command') and piece_id == 'terminal':
                        # For terminal commands, use command processor
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


class DesktopEngine:
    """Main engine that coordinates all desktop plugins following TPM architecture"""
    
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
        
        print("\n=== PyOS Desktop Engine with TPM Architecture Active ===")
        print("Check pieces/master_ledger/ledger.txt for audit trail")
        print("Press Ctrl+C to quit")
        print("="*50)
    
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
    
    def _initialize_core_pieces(self):
        """Initialize core pieces if they don't exist"""
        # Create desktop piece directory and file
        desktop_dir = self.pieces_dir / "desktop"
        desktop_dir.mkdir(exist_ok=True)
        
        desktop_file = desktop_dir / "piece.txt"
        if not desktop_file.exists():
            with open(desktop_file, 'w') as f:
                f.write("""# Piece: desktop
state:
  visible: true
  windows: []
  position_x: 0
  position_y: 0
  context_menu_visible: false
methods: show_context_menu, hide_context_menu, add_window, remove_window, open_terminal
event_listeners: right_click, window_created, window_destroyed, context_menu_requested
responses:
  right_click: Desktop right-clicked, showing context menu
  window_created: Window added to desktop
  window_destroyed: Window removed from desktop
  default: Desktop event processed
""")

        # Create context_menu piece directory and file
        context_menu_dir = self.pieces_dir / "context_menu"
        context_menu_dir.mkdir(exist_ok=True)
        
        context_menu_file = context_menu_dir / "piece.txt"
        if not context_menu_file.exists():
            with open(context_menu_file, 'w') as f:
                f.write("""# Piece: context_menu
state:
  visible: false
  position_x: 0
  position_y: 0
  selected_option: ""
  options: ["open_terminal", "show_apps", "settings"]
methods: show_at_position, hide, select_option, execute_selected
event_listeners: context_menu_shown, context_menu_hidden, option_selected
responses:
  context_menu_shown: Context menu displayed
  context_menu_hidden: Context menu hidden
  option_selected: Option selected in context menu
  default: Context menu event processed
""")

        # Create terminal piece directory and file
        terminal_dir = self.pieces_dir / "terminal"
        terminal_dir.mkdir(exist_ok=True)
        
        terminal_file = terminal_dir / "piece.txt"
        if not terminal_file.exists():
            with open(terminal_file, 'w') as f:
                f.write("""# Piece: terminal
state:
  visible: true
  content: ""
  command_history: []
  current_command: ""
  prompt: "$ "
methods: execute_command, display_message, display_error, clear_history
event_listeners: command_entered, command_executed, terminal_opened
responses:
  command_executed: Command processed in terminal
  terminal_opened: Terminal window opened
  default: Terminal event processed
""")

        # Create master ledger piece directory and file
        master_ledger_dir = self.pieces_dir / "master_ledger"
        master_ledger_dir.mkdir(exist_ok=True)
        
        # Create the ledger log file
        ledger_file = master_ledger_dir / "ledger.txt"
        if not ledger_file.exists():
            with open(ledger_file, 'w') as f:
                f.write("# Master Ledger - Central Audit Trail\n")
    
    def _load_plugins(self):
        """Dynamically load plugins from the plugins/ directories"""
        import importlib.util
        import os
        
        # First, load core plugins (like piece manager) that other plugins might depend on
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
                    print(f"Error loading core plugin {plugin_file}: {e}")
        
        # Then load desktop plugins
        desktop_plugins_path = Path("plugins/desktop")
        for plugin_file in desktop_plugins_path.glob("*.py"):
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
                        self.plugin_manager.register_plugin(f'desktop.{plugin_name}', plugin_instance)
                    else:
                        print(f"Warning: No Plugin class found in {plugin_file}")
                        
                except Exception as e:
                    print(f"Error loading desktop plugin {plugin_file}: {e}")
    
    def run(self):
        """Run the main loop with event processing - though now handled by Qt timer"""
        print("\nPyOS Desktop Engine running...\n")
        # This method now just initializes the necessary components
        # Actual processing is handled by Qt timer in main_tpm.py