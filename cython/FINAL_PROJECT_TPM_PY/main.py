"""
ASCII Roguelike Game Engine - TPM Compliant
Combines map display and menu system in a unified interface
"""

import json
import sys
import os
import tty
import termios
import select
import time
import importlib.util
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
    
    def render_all_plugins(self, screen=None):
        """Let all plugins render to screen"""
        for plugin in self.plugin_instances:
            try:
                plugin.render(screen)
            except AttributeError:
                # Plugin doesn't have render method, which is fine
                pass
    
    def find_plugin_by_capability(self, capability_attr):
        """Find a plugin that has a specific attribute/method"""
        for plugin in self.plugin_instances:
            if hasattr(plugin, capability_attr):
                return plugin
        return None


class RogueLikeEngine:
    """Main roguelike engine that coordinates all plugins"""
    
    def __init__(self):
        self.plugin_manager = PluginManager()
        self.running = True
        
        # Initialize the pieces directory
        self.pieces_dir = Path("games/default/pieces")
        self.pieces_dir.mkdir(parents=True, exist_ok=True)
        
        # Create required pieces
        self._create_initial_pieces()
        
        # Load plugins from configuration
        import sys
        import os
        current_dir = os.path.dirname(os.path.abspath(__file__))
        sys.path.insert(0, current_dir)
        
        # Load plugins from configuration
        config_file = "rogue_config.json"
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
                    if plugin_name == 'core.control':
                        self.control_plugin = plugin_instance
                    elif plugin_name == 'ui.display':
                        self.display_plugin = plugin_instance
                    elif plugin_name == 'core.piece_manager':
                        self.piece_manager = plugin_instance
                    elif plugin_name == 'core.keyboard':
                        self.keyboard_plugin = plugin_instance
                    elif plugin_name == 'core.event_manager':
                        self.event_manager = plugin_instance
                    elif plugin_name == 'core.player':
                        self.player_plugin = plugin_instance
                    elif plugin_name == 'core.world':
                        self.world_plugin = plugin_instance
                    elif plugin_name == 'core.npc':
                        self.npc_plugin = plugin_instance
                else:
                    print(f"Warning: Plugin {plugin_name} path not found in config")
        else:
            print("Config file not found")
        
        # Initialize all plugins
        self.plugin_manager.initialize_all_plugins()
    
    def _create_initial_pieces(self):
        """Create the initial piece files if they don't exist"""
        # Create player piece
        player_dir = self.pieces_dir / "player1"
        player_dir.mkdir(exist_ok=True)
        
        player_file = player_dir / "player1.txt"
        if not player_file.exists():
            with open(player_file, 'w') as f:
                f.write("""x 0
y 0
symbol @
health 100
inventory_count 0
active true
event_listeners [key_press, position_change]
methods [move, take_damage, heal]
status active
""")
        
        # Create world piece directory and files if they don't exist
        # They should already exist from setup
        world_dir = self.pieces_dir / "world"
        world_dir.mkdir(exist_ok=True)
        
        world_file = world_dir / "world.txt"
        map_file = world_dir / "map.txt"
        
        # These files should already exist, but create them if they don't
        if not world_file.exists():
            with open(world_file, 'w') as f:
                f.write("""width 50
height 20
terrain world
name Main World Map
type terrain
objects_count 0
event_listeners [position_change, move_attempt]
methods [load_world_from_files, get_tile, set_tile, render, initialize]
status active
map_file map.txt
""")
        
        if not map_file.exists():
            with open(map_file, 'w') as f:
                f.write("""#                  #          #                  #
#  .....T......... # ........ # ......T........  #
#  .....T......... # ........ # ......T........  #
#  ...............T# ........ # ..............T  #
#  ..........####..# ........ # ....~..........  #
#  ..........####..# ........ # ....~..........  #
#  ..........####..# ........ # ....~..........  #
#  ..........####..# ........ # ....~..........  #
#  ...............T# ........ # ......T........  #
#  ...............T# ........ # ......T........  #
#  ...............T# ........ # ......T........  #
#  ...............T# ........ # ......T........  #
#  ...............T# ........ # ......T........  #
#  ....R..........T# ........ # ......T........  #
#  ....R..........T# ........ # ......T........  #
#  ....R..........T# ........ # ......T........  #
#  ....R..........T# ........ # ......T........  #
#  ................# ........ # .............Z.  #
#                  #          #                  #
##################################################
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
        
        # Create clock piece
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
        
        # Create menu piece
        menu_dir = self.pieces_dir / "menu"
        menu_dir.mkdir(exist_ok=True)
        
        menu_file = menu_dir / "menu.txt"
        if not menu_file.exists():
            with open(menu_file, 'w') as f:
                f.write("""option_0 end_turn
option_1 hunger
option_2 state
option_3 act
option_4 inventory
option_count 5
current_selection -1
event_listeners [menu_select]
methods [show_menu, handle_selection, get_options]
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

    def run(self):
        """Run the main game loop"""
        print("ASCII Roguelike Game Engine started...")
        print("Use WASD or arrow keys to move, number keys for menu options")
        print("Press Ctrl+C to exit")
        print()
        
        # Refresh display initially
        if hasattr(self, 'display_plugin'):
            self.display_plugin.refresh_display()
        
        # Setup terminal for raw input
        old_settings = None
        try:
            old_settings = termios.tcgetattr(sys.stdin)
            tty.setcbreak(sys.stdin.fileno())
        except:
            print("Warning: Could not setup raw input mode")
        
        try:
            last_render_time = time.time()
            last_player_pos = None
            last_menu_selection = None
            
            # Get initial player position
            if hasattr(self, 'player_plugin') and hasattr(self.player_plugin, 'get_player_position'):
                initial_pos = self.player_plugin.get_player_position()
                if initial_pos:
                    last_player_pos = [initial_pos[0], initial_pos[1]]  # Create a copy
                else:
                    last_player_pos = None
            
            while self.running:
                # Check exit condition from control plugin
                if hasattr(self, 'piece_manager'):
                    control_state = self.piece_manager.get_piece_state('keyboard')  # Using keyboard piece for quit indicator
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
                
                # Update plugins
                current_time = time.time()
                dt = current_time - last_render_time
                self.plugin_manager.update_all_plugins(dt)
                
                # Check if we need to update the display
                needs_refresh = False
                
                # Check if player moved
                if hasattr(self, 'player_plugin') and hasattr(self.player_plugin, 'get_player_position'):
                    current_player_pos = self.player_plugin.get_player_position()
                    if current_player_pos and last_player_pos and current_player_pos != last_player_pos:
                        needs_refresh = True
                        last_player_pos = [current_player_pos[0], current_player_pos[1]]  # Create a copy
                
                # Check if there was a menu selection
                if hasattr(self, 'piece_manager'):
                    control_state = self.piece_manager.get_piece_state('keyboard')
                    if control_state and 'last_menu_choice' in control_state:
                        current_menu_choice = control_state.get('last_menu_choice')
                        if current_menu_choice != last_menu_selection and current_menu_choice is not None:
                            needs_refresh = True
                            last_menu_selection = current_menu_choice
                
                # Update display only when needed
                if needs_refresh and hasattr(self, 'display_plugin'):
                    self.display_plugin.refresh_display()
                
                last_render_time = current_time
                time.sleep(0.01)  # Small delay to prevent excessive CPU usage
        
        finally:
            # Restore terminal
            if old_settings:
                try:
                    termios.tcsetattr(sys.stdin, termios.TCSADRAIN, old_settings)
                except:
                    pass
        
        print("ASCII Roguelike Game Engine shutting down...")


def run_game():
    """Run the game engine directly"""
    engine = RogueLikeEngine()
    engine.run()


if __name__ == "__main__":
    engine = RogueLikeEngine()
    engine.run()