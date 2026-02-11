"""
Main Menu Engine - TPM Compliant Menu System
Uses the same plugin structure as the game but for menu functionality
"""

import json
import os
import sys
import importlib.util
from pathlib import Path

# Add the current directory to Python path to find plugin_interface
current_dir = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, current_dir)

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


class MenuEngine:
    """Main menu engine that coordinates all plugins"""
    
    def __init__(self):
        self.plugin_manager = PluginManager()
        self.running = True
        
        # Initialize the pieces directory
        self.pieces_dir = Path("games/default/pieces")
        self.pieces_dir.mkdir(parents=True, exist_ok=True)
        
        # Create menu pieces
        self._create_menu_pieces()
        
        # Loading plugins dynamically from config file like the original game
        import json
        import importlib.util
        import sys
        import os
        current_dir = os.path.dirname(os.path.abspath(__file__))
        sys.path.insert(0, current_dir)
        
        # Load plugins from configuration
        config_file = "menu_config.json"
        if os.path.exists(config_file):
            with open(config_file, 'r') as f:
                config = json.load(f)
            
            enabled_plugins = config.get('enabled_plugins', [])
            plugin_paths = config.get('plugin_paths', {})
            
            # Load each enabled plugin
            for plugin_name in enabled_plugins:
                if plugin_name in plugin_paths:
                    plugin_path = os.path.join(current_dir, plugin_paths[plugin_name])
                    
                    # Import the plugin module
                    spec = importlib.util.spec_from_file_location("plugin_module", plugin_path)
                    module = importlib.util.module_from_spec(spec)
                    spec.loader.exec_module(module)
                    
                    # Create an instance of the Plugin class from the module
                    plugin_instance = module.Plugin(game_engine=self)
                    
                    # Register the plugin instance
                    self.plugin_manager.register_plugin(plugin_name, plugin_instance)
                    
                    # Set specific references for specific plugins that are needed elsewhere
                    if plugin_name == 'core.menu_control':
                        self.control_plugin = plugin_instance
                    elif plugin_name == 'ui.menu_display':
                        self.display_plugin = plugin_instance
                    elif plugin_name == 'core.piece_manager':
                        self.piece_manager = plugin_instance
                    elif plugin_name == 'core.keyboard':
                        self.keyboard_plugin = plugin_instance
                    elif plugin_name == 'core.event_manager':
                        self.event_manager = plugin_instance
                else:
                    print(f"Warning: Plugin {plugin_name} path not found in config")
        else:
            # Fallback: create plugins directly if config doesn't exist
            from plugins.core.menu_control import Plugin as ControlPlugin
            from plugins.ui.menu_display import Plugin as DisplayPlugin
            from plugins.core.piece_manager import Plugin as PieceManagerPlugin
            from plugins.core.keyboard import Plugin as KeyboardPlugin
            from plugins.core.event_manager import Plugin as EventManagerPlugin
            
            # Create and register plugins
            self.piece_manager = PieceManagerPlugin(self)
            self.keyboard_plugin = KeyboardPlugin(self)
            self.control_plugin = ControlPlugin(self)
            self.display_plugin = DisplayPlugin(self)
            self.event_manager = EventManagerPlugin(self)  # Even though it's menu-focused, keep event system
            
            self.plugin_manager.register_plugin('core.piece_manager', self.piece_manager)
            self.plugin_manager.register_plugin('core.keyboard', self.keyboard_plugin)
            self.plugin_manager.register_plugin('core.menu_control', self.control_plugin)
            self.plugin_manager.register_plugin('ui.menu_display', self.display_plugin)
            self.plugin_manager.register_plugin('core.event_manager', self.event_manager)
        
        # Initialize all plugins
        self.plugin_manager.initialize_all_plugins()
        
        # Refresh display initially
        self.display_plugin.refresh_display()
    
    def _create_menu_pieces(self):
        """Create the menu piece files if they don't exist"""
        # Create main menu piece
        main_menu_dir = self.pieces_dir / "main_menu"
        main_menu_dir.mkdir(exist_ok=True)
        
        main_menu_file = main_menu_dir / "main_menu.txt"
        if not main_menu_file.exists():
            with open(main_menu_file, 'w') as f:
                f.write("""title Main Menu
option_0 Open Submenu
option_1 Settings
option_2 Utilities
option_3 Help
option_4 Exit
option_count 5
current_selection 0
event_listeners [key_press, menu_select]
methods [show_menu, handle_selection, navigate_menu]
status active
parent_menu none
""")
        
        # Create submenu piece
        submenu_dir = self.pieces_dir / "submenu"
        submenu_dir.mkdir(exist_ok=True)
        
        submenu_file = submenu_dir / "submenu.txt"
        if not submenu_file.exists():
            with open(submenu_file, 'w') as f:
                f.write("""title Sub Menu
option_0 Option 1
option_1 Option 2
option_2 Option 3
option_3 Back to Main
option_count 4
current_selection 0
event_listeners [key_press, menu_select]
methods [show_menu, handle_selection, navigate_menu]
status active
parent_menu main_menu
""")
        
        # Create settings menu piece
        settings_menu_dir = self.pieces_dir / "settings_menu"
        settings_menu_dir.mkdir(exist_ok=True)
        
        settings_menu_file = settings_menu_dir / "settings_menu.txt"
        if not settings_menu_file.exists():
            with open(settings_menu_file, 'w') as f:
                f.write("""title Settings Menu
option_0 Audio
option_1 Video
option_2 Controls
option_3 Back to Main
option_count 4
current_selection 0
event_listeners [key_press, menu_select]
methods [show_menu, handle_selection, navigate_menu]
status active
parent_menu main_menu
""")
        
        # Create utilities menu piece
        utilities_menu_dir = self.pieces_dir / "utilities_menu"
        utilities_menu_dir.mkdir(exist_ok=True)
        
        utilities_menu_file = utilities_menu_dir / "utilities_menu.txt"
        if not utilities_menu_file.exists():
            with open(utilities_menu_file, 'w') as f:
                f.write("""title Utilities Menu
option_0 File Manager
option_1 Text Editor
option_2 Calculator
option_3 Clock
option_4 Back to Main
option_count 5
current_selection 0
event_listeners [key_press, menu_select]
methods [show_menu, handle_selection, navigate_menu]
status active
parent_menu main_menu
""")
        
        # Create help menu piece
        help_menu_dir = self.pieces_dir / "help_menu"
        help_menu_dir.mkdir(exist_ok=True)
        
        help_menu_file = help_menu_dir / "help_menu.txt"
        if not help_menu_file.exists():
            with open(help_menu_file, 'w') as f:
                f.write("""title Help Menu
option_0 Instructions
option_1 Documentation
option_2 About
option_3 Back to Main
option_count 4
current_selection 0
event_listeners [key_press, menu_select]
methods [show_menu, handle_selection, navigate_menu]
status active
parent_menu main_menu
""")
        
        # Create menu control piece
        menu_control_dir = self.pieces_dir / "menu_control"
        menu_control_dir.mkdir(exist_ok=True)
        
        menu_control_file = menu_control_dir / "menu_control.txt"
        if not menu_control_file.exists():
            with open(menu_control_file, 'w') as f:
                f.write("""current_menu main_menu
current_selection 0
initialized false
supported_inputs [NUMBERS, ARROWS, NAVIGATION]
last_action none
quit_requested false
event_listeners [key_press]
methods [handle_selection, navigate_menu]
status active
""")
        
        # Create display piece
        display_dir = self.pieces_dir / "display"
        display_dir.mkdir(exist_ok=True)
        
        display_file = display_dir / "display.txt"
        if not display_file.exists():
            with open(display_file, 'w') as f:
                f.write("""last_refresh_time 0
refresh_count 0
active true
event_listeners [refresh_request]
methods [refresh_display]
status active
""")
        
        # Create keyboard piece
        keyboard_dir = self.pieces_dir / "keyboard"
        keyboard_dir.mkdir(exist_ok=True)
        
        keyboard_file = keyboard_dir / "keyboard.txt"
        if not keyboard_file.exists():
            with open(keyboard_file, 'w') as f:
                f.write("""session_id 12345
current_key_log_path games/default/pieces/keyboard/history.txt
logging_enabled true
keys_recorded 0
event_listeners [key_press]
last_key_logged 
last_key_time 
last_key_code 0
methods [log_key, reset_session, get_session_log, enable_logging, disable_logging]
status active
""")
        
        # Create clock piece (needed for event system)
        clock_dir = self.pieces_dir / "clock"
        clock_dir.mkdir(exist_ok=True)
        
        clock_file = clock_dir / "clock.txt"
        if not clock_file.exists():
            with open(clock_file, 'w') as f:
                f.write("""current_time 0
turn 0
status active
event_listeners [time_tick]
methods [tick, get_time, increment_turn]
""")
        
        # Create events piece (needed for event system)
        events_dir = self.pieces_dir / "clock_events"
        events_dir.mkdir(exist_ok=True)
        
        events_file = events_dir / "clock_events.txt"
        if not events_file.exists():
            with open(events_file, 'w') as f:
                f.write("""event_0_time 0
event_0_method tick
event_0_target clock
recurring_interval 1
current_time 0
next_event_time 1
methods [register_event, execute_at_time, schedule_recurring, check_events]
""")

    def run(self):
        """Run the main menu loop"""
        print("Menu engine started...")
        import time
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
        
        try:
            last_update_time = time.time()
            while self.running:
                # Check exit condition from control plugin
                control_state = self.piece_manager.get_piece_state('menu_control')
                if control_state and control_state.get('quit_requested', False):
                    break
                
                # Handle any direct input (for immediate echo)
                try:
                    if select.select([sys.stdin], [], [], 0) == ([sys.stdin], [], []):
                        ch = sys.stdin.read(1)
                        # Echo immediately
                        print(ch, end='', flush=True)
                        # Pass to plugins for processing
                        self.plugin_manager.handle_input_for_all_plugins(ch)
                        
                        # If it's a carriage return, handle as command
                        if ch in ['\n', '\r']:
                            print()  # Add newline
                except:
                    pass
                
                # Update plugins periodically
                current_time = time.time()
                if current_time - last_update_time > 0.05:  # Update every 50ms
                    dt = current_time - last_update_time
                    self.plugin_manager.update_all_plugins(dt)
                    last_update_time = current_time
                
                time.sleep(0.01)  # Small delay to prevent excessive CPU usage
        
        finally:
            # Restore terminal
            if old_settings:
                try:
                    termios.tcsetattr(sys.stdin, termios.TCSADRAIN, old_settings)
                except:
                    pass
        
        print("Menu engine shutting down...")


def run_menu_engine():
    """Run the menu engine directly"""
    engine = MenuEngine()
    engine.run()


if __name__ == "__main__":
    engine = MenuEngine()
    engine.run()