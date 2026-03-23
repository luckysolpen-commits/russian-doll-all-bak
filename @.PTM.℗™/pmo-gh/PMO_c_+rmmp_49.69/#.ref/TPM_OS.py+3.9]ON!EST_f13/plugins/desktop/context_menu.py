"""
Context Menu Plugin for PyOS - Virtual Operating System (TPM Version)
Manages the Qt context menu that appears on desktop right-click
"""

from PySide6.QtWidgets import QMenu
from PySide6.QtGui import QAction
from PySide6.QtCore import Qt
from plugin_interface import PluginInterface


class Plugin(PluginInterface):
    def __init__(self, engine):
        super().__init__(engine)
        self.qt_app = None
        self.main_window = None
        self.desktop_widget = None
        self.context_menu = None
        self.piece_manager = None
        self.context_menu_piece_name = 'context_menu'
        self.desktop_piece_name = 'desktop'
        self.master_ledger_file = 'pieces/master_ledger/ledger.txt'
        
        # Track menu position
        self.menu_position = (0, 0)
    
    def initialize(self):
        """Initialize the context menu plugin"""
        print("Context Menu plugin initializing...")
        
        # Find the desktop plugin to get references to Qt widgets
        for plugin in self.engine.plugin_manager.plugin_instances:
            if hasattr(plugin, 'main_window') and hasattr(plugin, 'desktop_widget'):
                self.main_window = plugin.main_window
                self.desktop_widget = plugin.desktop_widget
                break
        
        # Find the piece manager plugin
        for plugin in self.engine.plugin_manager.plugin_instances:
            if hasattr(plugin, 'get_piece_state'):
                self.piece_manager = plugin
                break
        
        print("Context Menu plugin initialized")
    
    def handle_input(self, key):
        """Handle keyboard input"""
        pass
    
    def update(self, dt):
        """Update context menu state - check for right-click events from master ledger"""
        # Check if a right-click event has been logged in the master ledger
        # This is a simplified check - in a full implementation, we'd monitor the file efficiently
        try:
            with open(self.master_ledger_file, 'r') as ledger:
                lines = ledger.readlines()
                
            # Look for the most recent right-click event
            for line in reversed(lines):
                if 'EventFire: right_click on desktop' in line and 'Source: user_input' in line:
                    # Extract position if available (for now we'll use cursor position)
                    self.show_context_menu()
                    break
        except FileNotFoundError:
            pass
    
    def show_context_menu(self):
        """Show the context menu at the current mouse position"""
        if self.desktop_widget:
            # Create the context menu
            self.context_menu = QMenu(self.desktop_widget)
            
            # Add terminal option
            terminal_action = QAction("Open Terminal", self.desktop_widget)
            terminal_action.triggered.connect(self.open_terminal)
            self.context_menu.addAction(terminal_action)
            
            # Add screensaver option
            screensaver_action = QAction("Show Screensaver", self.desktop_widget)
            screensaver_action.triggered.connect(self.show_screensaver)
            self.context_menu.addAction(screensaver_action)
            
            # Add hangman game option
            hangman_action = QAction("Play Hangman", self.desktop_widget)
            hangman_action.triggered.connect(self.show_hangman)
            self.context_menu.addAction(hangman_action)
            
            # Add PyBoard options
            pyboard2d_action = QAction("2D PyBoard", self.desktop_widget)
            pyboard2d_action.triggered.connect(self.show_pyboard2d)
            self.context_menu.addAction(pyboard2d_action)
            
            pyboard3d_action = QAction("3D PyBoard", self.desktop_widget)
            pyboard3d_action.triggered.connect(self.show_pyboard3d)
            self.context_menu.addAction(pyboard3d_action)
            
            # Add other application options
            apps_action = QAction("Open Apps Menu", self.desktop_widget)
            apps_action.triggered.connect(self.open_apps_menu)
            self.context_menu.addAction(apps_action)
            
            # Show the menu at the cursor position
            self.context_menu.popup(self.desktop_widget.mapToGlobal(self.get_mouse_position()))
    
    def get_mouse_position(self):
        """Get the current mouse position relative to the desktop widget"""
        # For now, return a default position if we can't determine the actual cursor position
        from PySide6.QtCore import QPoint
        import QCursor
        try:
            # This may not work directly without QApplication being active
            from PySide6.QtGui import QCursor
            global_pos = QCursor.pos()
            local_pos = self.desktop_widget.mapFromGlobal(global_pos)
            return local_pos
        except:
            # Return center of desktop as fallback
            from PySide6.QtCore import QPoint
            return QPoint(self.desktop_widget.width() // 2, self.desktop_widget.height() // 2)
    
    def open_terminal(self):
        """Open a new terminal window via master ledger event"""
        import time
        timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
        
        # Log terminal opening event to master ledger
        log_entry = f"[{timestamp}] EventFire: open_terminal on context_menu | Source: user_selection\n"
        with open(self.master_ledger_file, 'a') as ledger:
            ledger.write(log_entry)
    
    def show_screensaver(self):
        """Show the screensaver via master ledger event"""
        import time
        timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
        
        # Log screensaver event to master ledger
        log_entry = f"[{timestamp}] EventFire: show_screensaver on context_menu | Source: user_selection\n"
        with open(self.master_ledger_file, 'a') as ledger:
            ledger.write(log_entry)
    
    def show_hangman(self):
        """Show the hangman game via master ledger event"""
        import time
        timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
        
        # Log hangman event to master ledger
        log_entry = f"[{timestamp}] EventFire: show_hangman on context_menu | Source: user_selection\n"
        with open(self.master_ledger_file, 'a') as ledger:
            ledger.write(log_entry)
    
    def show_pyboard2d(self):
        """Show the 2D PyBoard via master ledger event"""
        import time
        timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
        
        # Log pyboard2d event to master ledger
        log_entry = f"[{timestamp}] EventFire: show_pyboard2d on context_menu | Source: user_selection\n"
        with open(self.master_ledger_file, 'a') as ledger:
            ledger.write(log_entry)
    
    def show_pyboard3d(self):
        """Show the 3D PyBoard via master ledger event"""
        import time
        timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
        
        # Log pyboard3d event to master ledger
        log_entry = f"[{timestamp}] EventFire: show_pyboard3d on context_menu | Source: user_selection\n"
        with open(self.master_ledger_file, 'a') as ledger:
            ledger.write(log_entry)
    
    def open_apps_menu(self):
        """Open the apps menu via master ledger event"""
        import time
        timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
        
        # Log apps menu event to master ledger
        log_entry = f"[{timestamp}] EventFire: open_apps_menu on context_menu | Source: user_selection\n"
        with open(self.master_ledger_file, 'a') as ledger:
            ledger.write(log_entry)
    
    def hide_context_menu(self):
        """Hide the context menu"""
        if self.context_menu:
            self.context_menu.close()