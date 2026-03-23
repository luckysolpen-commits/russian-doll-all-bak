"""
Desktop Plugin for PyOS - Virtual Operating System (TPM Version)
Manages the Qt desktop interface and responds to events
"""

from PySide6.QtWidgets import QApplication, QMainWindow, QWidget, QHBoxLayout, QLabel, QMenu, QMenuBar, QStatusBar
from PySide6.QtCore import Qt, QTimer
from PySide6.QtGui import QPalette, QAction, QColor
from plugin_interface import PluginInterface


class Plugin(PluginInterface):
    def __init__(self, engine):
        super().__init__(engine)
        self.qt_app = None
        self.main_window = None
        self.desktop_widget = None
        self.piece_manager = None
        self.desktop_piece_name = 'desktop'
        self.context_menu_piece_name = 'context_menu'
        self.master_ledger_file = 'pieces/master_ledger/ledger.txt'
        
        # Initialize Qt components
        self.init_qt_components()
    
    def init_qt_components(self):
        """Initialize Qt components for the desktop"""
        # Check if QApplication already exists; if not, create one
        app = QApplication.instance()
        if app is None:
            print("Creating new QApplication instance for desktop plugin")
            self.qt_app = QApplication([])
        else:
            self.qt_app = app
    
    def initialize(self):
        """Initialize the desktop plugin and create the main window"""
        print("Desktop plugin initializing...")
        
        # Find the piece manager plugin
        for plugin in self.engine.plugin_manager.plugin_instances:
            if hasattr(plugin, 'get_piece_state'):
                self.piece_manager = plugin
                break
        
        # Create the main window and desktop
        self.create_main_window()
        self.create_desktop_widget()
        
        # Connect the desktop to context menu functionality
        self.connect_desktop_context_menu()
        
        # Show the desktop window
        self.show()
        
        print("Desktop plugin initialized successfully")
    
    def create_main_window(self):
        """Create the main application window"""
        # Dynamically import to avoid circular dependencies at startup
        import sys
        from PySide6.QtWidgets import QApplication
        
        # Check if QApplication exists
        app = QApplication.instance()
        if app is None:
            print("Creating QApplication in desktop plugin")
            self.qt_app = QApplication(sys.argv)
        else:
            self.qt_app = app
            
        # Import and create main window
        from window_manager.main_window import MainWindow
        self.main_window = MainWindow()
        self.main_window.setWindowTitle("PyOS - Virtual Operating System (TPM)")
    
    def create_desktop_widget(self):
        """Create the desktop widget"""
        from window_manager.desktop import DesktopWidget
        self.desktop_widget = DesktopWidget()
        self.main_window.setCentralWidget(self.desktop_widget)
        
        # Update piece state to reflect desktop is visible
        if self.piece_manager:
            self.piece_manager.update_piece_state('desktop', {'visible': True})
    
    def connect_desktop_context_menu(self):
        """Connect the desktop to context menu functionality"""
        if self.desktop_widget:
            # Override the desktop widget's context menu to use our TPM-based system
            self.desktop_widget.setContextMenuPolicy(Qt.CustomContextMenu)
            self.desktop_widget.customContextMenuRequested.connect(self.handle_desktop_right_click)
    
    def handle_desktop_right_click(self, position):
        """Handle right-click on desktop - log to history and master ledger to trigger context menu"""
        import time
        timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
        
        # Log right-click event to desktop history file first (per TPM principle: all input recorded)
        history_entry = f"[{timestamp}] RightClick: ({position.x()}, {position.y()}) | Source: user_input\n"
        with open('pieces/desktop/history.txt', 'a') as history:
            history.write(history_entry)
        
        # Log right-click event to master ledger for processing
        log_entry = f"[{timestamp}] EventFire: right_click on desktop | Source: user_input | Position: ({position.x()}, {position.y()})\n"
        with open(self.master_ledger_file, 'a') as ledger:
            ledger.write(log_entry)
        
        # Update desktop piece state to indicate context menu is requested
        if self.piece_manager:
            self.piece_manager.update_piece_state('desktop', {'context_menu_visible': True})
    
    def handle_input(self, key):
        """Handle keyboard input"""
        # Desktop might handle certain global key combinations
        if key.lower() == '`':  # For example, maybe a special hotkey
            import time
            timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
            log_entry = f"[{timestamp}] EventFire: special_key_pressed on desktop | Key: {key}\n"
            with open(self.master_ledger_file, 'a') as ledger:
                ledger.write(log_entry)
    
    def update(self, dt):
        """Update desktop state"""
        # Update desktop state if needed
        pass
    
    def show(self):
        """Show the main window"""
        if self.main_window:
            self.main_window.show()
            
            # Process Qt events to ensure the window appears
            if self.qt_app:
                self.qt_app.processEvents()
    
    def hide(self):
        """Hide the main window"""
        if self.main_window:
            self.main_window.hide()