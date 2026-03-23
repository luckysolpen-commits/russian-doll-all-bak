#!/usr/bin/env python3
"""
Main entry point for PyOS - Virtual Operating System (TPM Version)
This implements the True Piece Method architecture with pieces, master ledger, and plugins
"""

import sys
import time
from PySide6.QtWidgets import QApplication
from engine import DesktopEngine

def main():
    """Main entry point for PyOS TPM"""
    print("Starting PyOS with True Piece Method (TPM) architecture...")
    
    # Initialize the Qt application
    app = QApplication.instance()
    if app is None:
        app = QApplication(sys.argv)
    
    # Create the desktop engine which manages all pieces and plugins
    engine = DesktopEngine()
    
    # Show the desktop after engine is initialized
    desktop_plugin = None
    for plugin in engine.plugin_manager.plugin_instances:
        if hasattr(plugin, 'show') and hasattr(plugin, 'main_window'):
            desktop_plugin = plugin
            break
    
    if desktop_plugin:
        desktop_plugin.show()
    
    # Instead of running the engine's custom loop, we need to integrate with Qt's event loop
    # Create a timer to periodically process ledger entries
    from PySide6.QtCore import QTimer
    
    def process_ledger():
        engine.master_ledger_monitor.process_new_entries()
        for plugin in engine.plugin_manager.plugin_instances:
            try:
                plugin.update(0.01)  # Small time delta
            except AttributeError:
                pass
    
    # Create a timer to periodically check for ledger updates
    ledger_timer = QTimer()
    ledger_timer.timeout.connect(process_ledger)
    ledger_timer.start(10)  # Update every 10ms
    
    # Run the Qt application event loop
    sys.exit(app.exec())


if __name__ == "__main__":
    main()