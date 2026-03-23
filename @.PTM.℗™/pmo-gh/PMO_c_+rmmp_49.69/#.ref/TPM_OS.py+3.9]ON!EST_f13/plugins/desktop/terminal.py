"""
Terminal Plugin for PyOS - Virtual Operating System (TPM Version)
Manages the terminal application as a piece
"""

from plugin_interface import PluginInterface


class Plugin(PluginInterface):
    def __init__(self, engine):
        super().__init__(engine)
        self.piece_manager = None
        self.terminal_piece_name = 'terminal'
        self.master_ledger_file = 'pieces/master_ledger/ledger.txt'
        self.terminal_app_instance = None
        self.terminal_window = None
        
    def initialize(self):
        """Initialize the terminal plugin"""
        print("Terminal plugin initializing...")
        
        # Find the piece manager plugin
        for plugin in self.engine.plugin_manager.plugin_instances:
            if hasattr(plugin, 'get_piece_state'):
                self.piece_manager = plugin
                break
        
        # Initialize the terminal piece state if it doesn't exist
        if self.piece_manager:
            terminal_state = self.piece_manager.get_piece_state(self.terminal_piece_name)
            if not terminal_state:
                terminal_state = {
                    'visible': True,
                    'content': "",
                    'command_history': [],
                    'current_command': "",
                    'prompt': "$ "
                }
                self.piece_manager.update_piece_state(self.terminal_piece_name, terminal_state)
        
        print("Terminal plugin initialized")
    
    def handle_input(self, key):
        """Handle keyboard input - for terminal this would typically be handled differently"""
        # Terminal input is usually handled by the terminal app directly
        pass
    
    def update(self, dt):
        """Update terminal state - check for terminal-related events from master ledger"""
        # This would monitor for terminal-specific events in the master ledger
        try:
            with open(self.master_ledger_file, 'r') as ledger:
                lines = ledger.readlines()
                
            # Process recent terminal events
            for line in reversed(lines[-10:]):  # Check last 10 lines for efficiency
                if 'EventFire: open_terminal on context_menu' in line:
                    self.open_terminal()
                    break
                elif 'MethodCall: execute_command on terminal' in line:
                    # This would execute a command in the terminal
                    self.execute_terminal_command()
                    break
        except FileNotFoundError:
            pass
    
    def open_terminal(self):
        """Open the terminal application"""
        if self.terminal_app_instance is None:
            try:
                # Import the terminal app and create an instance
                from apps.terminal_app import TerminalApp
                from window_manager.mini_window import MiniWindow
                
                # Check if we can get the app manager from another plugin
                app_manager = None
                for plugin in self.engine.plugin_manager.plugin_instances:
                    if hasattr(plugin, 'app_manager'):
                        app_manager = plugin.app_manager
                        break
                
                self.terminal_app_instance = TerminalApp(app_manager=app_manager)
                self.terminal_window = MiniWindow(title="Terminal", content_widget=self.terminal_app_instance)
                
                # Find and show on the desktop
                desktop_plugin = None
                for plugin in self.engine.plugin_manager.plugin_instances:
                    if hasattr(plugin, 'desktop_widget'):
                        desktop_plugin = plugin
                        break
                
                if desktop_plugin and desktop_plugin.desktop_widget:
                    self.terminal_window.setParent(desktop_plugin.desktop_widget)
                    self.terminal_window.show()
                    
                    # Add to desktop's window list
                    desktop_plugin.desktop_widget.add_window(self.terminal_window)
                    
                    # Update terminal piece state
                    if self.piece_manager:
                        terminal_state = self.piece_manager.get_piece_state(self.terminal_piece_name)
                        if terminal_state:
                            terminal_state['visible'] = True
                            self.piece_manager.update_piece_state(self.terminal_piece_name, terminal_state)
                            
                    print("Terminal opened successfully")
                else:
                    print("Could not find desktop to attach terminal to")
            except ImportError:
                print("Error: Could not import terminal app components")
            except Exception as e:
                print(f"Error opening terminal: {str(e)}")
    
    def execute_terminal_command(self):
        """Execute a command in the terminal"""
        # This would handle execution of commands in the terminal
        print("Processing terminal command")
        # Update the terminal piece state with the command execution
        if self.piece_manager:
            terminal_state = self.piece_manager.get_piece_state(self.terminal_piece_name)
            if terminal_state:
                # Update with command execution info
                import time
                timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
                terminal_state['content'] += f"\n[CMD EXECUTED AT {timestamp}]"
                self.piece_manager.update_piece_state(self.terminal_piece_name, terminal_state)