#!/usr/bin/env python3
"""
Main entry point for PyOS - Virtual Operating System
"""

import sys
from PySide6.QtWidgets import QApplication, QMainWindow, QWidget, QVBoxLayout
from PySide6.QtCore import Qt
from PySide6.QtGui import QColor, QPainter, QBrush
from PySide6.QtOpenGLWidgets import QOpenGLWidget
from PySide6.QtCore import QCoreApplication
from PySide6.QtGui import QSurfaceFormat
from OpenGL.GL import *
import OpenGL.GL
import OpenGL.arrays.vbo
import ctypes

# Set application attributes for OpenGL compatibility BEFORE creating QApplication
# This is critical for avoiding OpenGL context issues
QApplication.setAttribute(Qt.AA_UseDesktopOpenGL)
QApplication.setAttribute(Qt.AA_ShareOpenGLContexts)

from window_manager.main_window import MainWindow
from apps.app_manager import AppManager
from apps.settings_app import SettingsApp
from apps.profiles_app import ProfilesApp
from apps.wallet_app import WalletApp
from apps.store_app import StoreApp
from apps.msr_app import MsrApp
from apps.terminal_app import TerminalApp
from apps.cube_app import CubeApp
from apps.hangman_app import HangmanApp
from apps.pyboard2d_app import PyBoard2DApp
from apps.pyboard3d_app import PyBoard3DApp
from apps.screensaver_app import ScreensaverApp
from graphics.cube_screensaver import CubeScreensaver
from graphics.opengl_context_manager import opengl_manager


def configure_opengl_format():
    """Configure the OpenGL format before creating any OpenGL widgets"""
    print("[INFO] Configuring OpenGL format...")
    
    # Create a surface format with conservative settings
    format = QSurfaceFormat()
    
    # Conservative OpenGL version that should be widely supported
    format.setVersion(2, 1)  # OpenGL 2.1 for broader compatibility
    # Don't set a specific profile to avoid incompatibilities
    format.setDepthBufferSize(24)
    # Only enable multisampling if needed - this can cause issues
    # format.setSamples(4)  # Comment out for now to avoid potential issues
    
    # Set this as the default format for all OpenGL contexts
    QSurfaceFormat.setDefaultFormat(format)
    
    print("[SUCCESS] OpenGL format configured")


def main():
    # Initialize the main application
    app = QApplication(sys.argv)
    
    # Configure OpenGL format after QApplication is created
    configure_opengl_format()
    
    # Initialize OpenGL context early to prevent first-run crashes
    print("Initializing OpenGL context (this may take a moment)...")
    try:
        # Use our new OpenGL context manager for proper initialization
        success = opengl_manager.initialize_opengl_context()
        if success:
            print("OpenGL context initialization completed successfully")
        else:
            print("Warning: OpenGL context initialization failed, continuing startup...")
    except Exception as e:
        print(f"Warning: OpenGL context warmup failed: {e}, continuing startup...")
    
    # Now continue with the main application setup
    
    # Create the main app manager and register all apps
    app_manager = AppManager()
    app_manager.register_app('settings', SettingsApp)
    app_manager.register_app('profiles', ProfilesApp)
    app_manager.register_app('wallet', WalletApp)
    app_manager.register_app('store', StoreApp)
    app_manager.register_app('msr', MsrApp)
    app_manager.register_app('terminal', TerminalApp)
    app_manager.register_app('cube', CubeApp)  # Now using the Qt-only cube app
    app_manager.register_app('hangman', HangmanApp)
    app_manager.register_app('pyboard2d', PyBoard2DApp)
    app_manager.register_app('pyboard3d', PyBoard3DApp)
    app_manager.register_app('screensaver', ScreensaverApp)
    
    # Create the main window and assign the app manager
    main_window = MainWindow()
    main_window.app_manager = app_manager
    
    main_window.show()
    
    # Store the context manager and its widgets to keep them alive
    app.opengl_context_manager = opengl_manager
    app.opengl_dummy_widget, app.opengl_cube_widget = opengl_manager.get_managed_widgets()
    app.master_gl_context = opengl_manager.get_shared_gl_context() # Keep the master QOpenGLContext alive
    
    # Integrate the CubeScreensaver directly into the main_window for OpenGL context sanity test
    print("Integrating CubeScreensaver into main window for OpenGL context sanity test...")
    warmup_screensaver = CubeScreensaver()
    # Add it to the desktop layout (or main_layout of central widget)
    main_window.desktop.layout.addWidget(warmup_screensaver)
    warmup_screensaver.hide() # Keep it hidden
    app.warmup_screensaver_instance = warmup_screensaver # Keep a reference to prevent garbage collection
    
    # Process events to ensure its initializeGL is called within the main window's context
    QApplication.processEvents()
    
    sys.exit(app.exec())


if __name__ == "__main__":
    main()