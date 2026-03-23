"""
OpenGL Context Manager for PyOS
Handles proper initialization of OpenGL contexts to prevent first-run issues
"""

from PySide6.QtWidgets import QApplication
from PySide6.QtOpenGLWidgets import QOpenGLWidget
from PySide6.QtGui import QOpenGLContext, QSurfaceFormat # Import QOpenGLContext and QSurfaceFormat
from OpenGL.GL import *
import OpenGL.GL
import OpenGL.arrays.vbo
import ctypes
from graphics.cube_screensaver import CubeScreensaver


class OpenGLContextManager:
    """Manages OpenGL contexts for PyOS to prevent first-run issues"""
    
    def __init__(self):
        self.opengl_initialized = False
        self.dummy_widget = None
        self.cube_widget = None
        self._master_gl_context = QOpenGLContext() # Create a master QOpenGLContext
        
    def initialize_opengl_context(self):
        """Initialize OpenGL context with dummy operations to prevent first-run issues"""
        if self.opengl_initialized:
            print("[INFO] OpenGL context already initialized")
            return True
            
        try:
            print("[INFO] Initializing OpenGL context for PyOS... This will create a master shared context.")
            
            # Create a minimal OpenGL widget for context initialization first
            # This widget will create its own context, which we will use for sharing
            self.dummy_widget = self._create_dummy_opengl_widget()
            self.dummy_widget.show() # Show to trigger its internal context creation
            self.dummy_widget.hide() # Hide immediately
            
            # Make the dummy widget's context current first
            self.dummy_widget.makeCurrent()
            self.dummy_widget.initializeGL() # Initialize the dummy widget's GL context
            
            # Now, set the format for the master context
            format = QSurfaceFormat()
            format.setVersion(2, 1)
            format.setDepthBufferSize(24)
            self._master_gl_context.setFormat(format)
            
            # Set the master context to share with the dummy widget's context
            self._master_gl_context.setShareContext(self.dummy_widget.context())
            
            # Create the master context (no need to make it current if its only purpose is sharing)
            if not self._master_gl_context.create():
                raise Exception("Failed to create master OpenGL context!")
            
            # Initialize and paint the dummy widget using the now-shared master context
            self.dummy_widget.paintGL() # Already made current and initializedGL
            
            # Also create and initialize the cube screensaver for comprehensive warmup
            self.cube_widget = CubeScreensaver()
            # Its context will also share with the master context
            self.cube_widget.context().setShareContext(self._master_gl_context)
            self.cube_widget.show()
            self.cube_widget.hide()
            self.cube_widget.makeCurrent()
            self.cube_widget.initializeGL()
            self.cube_widget.paintGL()
            
            # Process events to ensure everything is initialized
            QApplication.processEvents()
            
            # Clean up references properly
            self.opengl_initialized = True
            print("[SUCCESS] OpenGL master shared context created and initialized successfully")
            
            return True
        except Exception as e:
            print(f"[ERROR] Failed to initialize OpenGL context: {e}")
            return False
    
    def _create_dummy_opengl_widget(self, parent=None): # Accept parent here
        """Create a minimal OpenGL widget for context initialization"""
        class DummyOpenGLWidget(QOpenGLWidget):
            def __init__(self, parent=None): # Add parent
                super().__init__(parent=parent)
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
        
        return DummyOpenGLWidget(parent=parent) # Pass parent to constructor
    
    def get_managed_widgets(self):
        """Return the initialized widgets to keep them alive"""
        return self.dummy_widget, self.cube_widget
        
    def get_shared_gl_context(self):
        """Return the shared QOpenGLContext instance managed by the manager"""
        return self._master_gl_context


# Global instance to manage OpenGL contexts
opengl_manager = OpenGLContextManager()