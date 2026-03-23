"""
OpenGL Context Warmup Utility for PyOS
This module provides functionality to initialize an OpenGL context early in PyOS startup
to prevent crash-on-first-open issues with OpenGL applications.
"""

import sys
from PySide6.QtWidgets import QApplication, QOpenGLWidget
from PySide6.QtCore import QTimer
from PySide6.QtOpenGLWidgets import QOpenGLWidget
from OpenGL.GL import *
import OpenGL.GL
import OpenGL.arrays.vbo
import ctypes


class DummyOpenGLWidget(QOpenGLWidget):
    """
    A minimal OpenGL widget used solely for context initialization
    """
    def __init__(self):
        super().__init__()
        self.setHidden(True)  # Keep hidden from user
        self.resize(1, 1)  # Minimal size to reduce resource usage
        self.context_initialized = False

    def initializeGL(self):
        """Initialize OpenGL context with basic settings"""
        try:
            # Basic OpenGL initialization
            glClearColor(0.0, 0.0, 0.0, 0.0)  # Black background
            glEnable(GL_DEPTH_TEST)  # Enable depth testing
            glEnable(GL_BLEND)  # Enable blending
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
            glEnable(GL_CULL_FACE)  # Enable face culling
            glCullFace(GL_BACK)
            glShadeModel(GL_SMOOTH)  # Smooth shading
            
            # Set up basic projection
            glMatrixMode(GL_PROJECTION)
            glLoadIdentity()
            glOrtho(0, 1, 0, 1, -1, 1)
            
            glMatrixMode(GL_MODELVIEW)
            glLoadIdentity()
            
            self.context_initialized = True
            print("[DEBUG] OpenGL context initialized successfully")
        except Exception as e:
            print(f"[ERROR] Failed to initialize OpenGL context: {e}")

    def paintGL(self):
        """Basic paint operation to ensure context is usable"""
        try:
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
            # Simple drawing operation to make sure everything works
            glColor3f(1.0, 1.0, 1.0)  # White color
            glBegin(GL_POINTS)
            glVertex2f(0.5, 0.5)  # Single point in the middle
            glEnd()
        except Exception as e:
            print(f"[ERROR] Failed during paintGL: {e}")

    def resizeGL(self, width, height):
        """Handle resize"""
        glViewport(0, 0, width, height)


def warmup_opengl_context(app_instance=None):
    """
    Warm up the OpenGL context to avoid first-run crashes
    This can be called early in the PyOS startup process
    """
    try:
        # If no QApplication exists, create one temporarily
        temp_app = None
        if not QApplication.instance():
            temp_app = QApplication([sys.argv[0]] if len(sys.argv) > 0 else [""])
        
        # Create and initialize the dummy OpenGL widget
        print("[INFO] Initializing OpenGL context warmup...")
        dummy_widget = DummyOpenGLWidget()
        
        # Force the OpenGL context to be created
        dummy_widget.makeCurrent()
        dummy_widget.initializeGL()
        dummy_widget.paintGL()  # Execute a basic render operation
        
        # Clean up the temporary application if we created one
        if temp_app:
            dummy_widget.destroy()
            temp_app.quit()
        
        print("[SUCCESS] OpenGL context warmup completed successfully")
        return True
    except Exception as e:
        print(f"[ERROR] OpenGL context warmup failed: {e}")
        return False


def lazy_warmup_opengl_context(app_instance):
    """
    Alternative warmup that schedules itself to run later
    when the main application is already running
    """
    try:
        print("[INFO] Scheduling OpenGL context warmup...")
        
        # Create and schedule the warmup to happen after the event loop starts
        dummy_widget = DummyOpenGLWidget()
        
        # Use a single shot timer to make sure the GUI system has initialized
        def do_warmup():
            try:
                dummy_widget.show()
                dummy_widget.raise_()  # Bring to front
                dummy_widget.lower()  # Send back
                dummy_widget.hide()  # Hide again
                
                # Force the OpenGL context initialization
                dummy_widget.makeCurrent()
                dummy_widget.initializeGL()
                dummy_widget.paintGL()
                
                # Clean up
                dummy_widget.destroy()
                
                print("[SUCCESS] Lazy OpenGL context warmup completed")
            except Exception as e:
                print(f"[ERROR] Lazy OpenGL warmup failed: {e}")
        
        QTimer.singleShot(100, do_warmup)  # Wait 100ms then run
        return True
    except Exception as e:
        print(f"[ERROR] Failed to schedule lazy OpenGL warmup: {e}")
        return False


if __name__ == "__main__":
    # Demo: show how this would work
    app = QApplication(sys.argv)
    
    print("Testing OpenGL context warmup...")
    success = warmup_opengl_context(app)
    
    if success:
        print("OpenGL warmup successful!")
    else:
        print("OpenGL warmup failed!")
    
    # Simple exit after testing
    QTimer.singleShot(1000, app.quit)  # Exit after 1 second
    app.exec()