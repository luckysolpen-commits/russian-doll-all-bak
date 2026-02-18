#!/usr/bin/env python3
"""
GL Renderer - Qt/OpenGL window display with keyboard input
Reads from pieces/display/current_frame.txt (same as ASCII renderer)
Writes to pieces/keyboard/history.txt (same as keyboard.py)
Checks pieces/master_ledger/frame_changed.txt for updates
Based on hybrid_clean.0.py and final_fixed_game_v2.py Qt implementations
"""

import os
import sys
import time
from datetime import datetime

# PySide6 and OpenGL imports
try:
    from PySide6.QtWidgets import QApplication, QMainWindow, QLabel
    from PySide6.QtCore import Qt, QTimer
    from PySide6.QtGui import QColor, QFont
    from PySide6.QtOpenGLWidgets import QOpenGLWidget
    from OpenGL.GL import *
    from OpenGL.GLU import *
    from OpenGL.GLUT import *
except ImportError as e:
    sys.exit(1)

# Constants
WINDOW_WIDTH = 800
WINDOW_HEIGHT = 1200
BACKGROUND_COLOR = (0.1, 0.1, 0.1, 1.0)
TEXT_COLOR = (1.0, 1.0, 1.0, 1.0)

# File paths
script_dir = os.path.dirname(__file__)
CURRENT_FRAME_PATH = os.path.join(script_dir, '../pieces/display/current_frame.txt')
MARKER_FILE_PATH = os.path.join(script_dir, '../pieces/master_ledger/frame_changed.txt')
KEYBOARD_HISTORY_PATH = os.path.join(script_dir, '../pieces/keyboard/history.txt')
MASTER_LEDGER_PATH = os.path.join(script_dir, '../pieces/master_ledger/master_ledger.txt')
LOG_FILE_PATH = os.path.join(script_dir, '../plugins/logs.txt')


def log(msg):
    """Write to log file instead of stderr"""
    try:
        with open(LOG_FILE_PATH, 'a') as f:
            f.write(f"{msg}\n")
    except:
        pass


def write_keyboard_input(key_code):
    """Write key press to keyboard history (matches keyboard.py format - raw code only)"""
    try:
        os.makedirs(os.path.dirname(KEYBOARD_HISTORY_PATH), exist_ok=True)
        
        # Write to keyboard history (raw code only, no timestamp - matches keyboard.py)
        with open(KEYBOARD_HISTORY_PATH, 'a') as fp:
            fp.write(f"{key_code}\n")
        
        # Log to master ledger with timestamp
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        with open(MASTER_LEDGER_PATH, 'a') as ledger:
            ledger.write(f"[{timestamp}] InputReceived: key_code={key_code} | Source: gl_keyboard_input\n")
                
    except Exception as e:
        log(f"Error writing keyboard input: {e}")


class RenderGLWidget(QOpenGLWidget):
    def __init__(self):
        super().__init__()
        self.frame_content = ""
        self.lines = []
        self.setMinimumSize(WINDOW_WIDTH, WINDOW_HEIGHT)
        self.setFocusPolicy(Qt.StrongFocus)
    
    def set_frame_content(self, content):
        """Set the frame content and trigger repaint"""
        self.frame_content = content
        self.lines = content.split('\n')
        self.update()  # Trigger repaint
    
    def initializeGL(self):
        """Initialize OpenGL settings"""
        glClearColor(*BACKGROUND_COLOR)
        glEnable(GL_BLEND)
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
    
    def resizeGL(self, w, h):
        """Handle window resize"""
        glViewport(0, 0, w, h)
        glMatrixMode(GL_PROJECTION)
        glLoadIdentity()
        glOrtho(0, w, h, 0, -1, 1)  # Top-left origin for text rendering
        glMatrixMode(GL_MODELVIEW)
        glLoadIdentity()
    
    def paintGL(self):
        """Paint the frame content"""
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
        glLoadIdentity()
        
        # Set text color
        glColor3f(*TEXT_COLOR[:3])
        
        # Draw each line of text
        y_position = 50
        line_spacing = 22
        
        for line in self.lines:
            self.render_text(line, 50, y_position)
            y_position += line_spacing
    
    def render_text(self, text, x, y):
        """Render text using OpenGL bitmap fonts"""
        glRasterPos2f(x, y)
        for char in text:
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, ord(char))
    
    def keyPressEvent(self, event):
        """Handle keyboard input - direct write like C version"""
        key = event.key()
        
        # Map Qt key codes to our key codes (matching C version)
        key_code = 0
        
        if key == Qt.Key_Up:
            key_code = 1002
        elif key == Qt.Key_Down:
            key_code = 1003
        elif key == Qt.Key_Left:
            key_code = 1000
        elif key == Qt.Key_Right:
            key_code = 1001
        elif key == Qt.Key_W:
            key_code = ord('w')
        elif key == Qt.Key_S:
            key_code = ord('s')
        elif key == Qt.Key_A:
            key_code = ord('a')
        elif key == Qt.Key_D:
            key_code = ord('d')
        elif key == Qt.Key_1:
            key_code = ord('1')
        elif key == Qt.Key_2:
            key_code = ord('2')
        elif key == Qt.Key_3:
            key_code = ord('3')
        elif key == Qt.Key_4:
            key_code = ord('4')
        elif key == Qt.Key_5:
            key_code = ord('5')
        elif key == Qt.Key_6:
            key_code = ord('6')
        elif key == Qt.Key_C and event.modifiers() == Qt.ControlModifier:
            # Ctrl+C - close window
            QApplication.instance().quit()
            return
        elif key >= Qt.Key_A and key <= Qt.Key_Z:
            # Letters a-z
            key_code = ord('a') + (key - Qt.Key_A)
        elif key >= Qt.Key_0 and key <= Qt.Key_9:
            # Numbers 0-9
            key_code = ord('0') + (key - Qt.Key_0)
        else:
            # Try to get character
            text = event.text()
            if text:
                key_code = ord(text[0])
        
        if key_code > 0:
            write_keyboard_input(key_code)


class GLRendererWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        
        self.last_marker_size = 0
        
        # Set up central OpenGL widget
        self.gl_widget = RenderGLWidget()
        self.gl_widget.set_frame_content("")
        self.setCentralWidget(self.gl_widget)
        
        self.setWindowTitle("TPM FuzzPet - GL Renderer")
        self.setGeometry(100, 100, WINDOW_WIDTH, WINDOW_HEIGHT)
        
        # Timer for checking frame updates (60 FPS = 16.667ms)
        self.timer = QTimer()
        self.timer.timeout.connect(self.check_for_updates)
        self.timer.start(16)
        
        # Get initial marker size
        if os.path.exists(MARKER_FILE_PATH):
            self.last_marker_size = os.path.getsize(MARKER_FILE_PATH)
        
        # Initial frame load
        self.load_frame()
        
        # Make sure window can receive key events
        self.setFocusPolicy(Qt.StrongFocus)
        self.setFocus()
    
    def check_for_updates(self):
        """Check marker file for frame updates"""
        if os.path.exists(MARKER_FILE_PATH):
            current_size = os.path.getsize(MARKER_FILE_PATH)
            if current_size != self.last_marker_size:
                self.load_frame()
                self.last_marker_size = current_size
    
    def load_frame(self):
        """Load frame content from file"""
        try:
            if os.path.exists(CURRENT_FRAME_PATH):
                with open(CURRENT_FRAME_PATH, 'r', encoding='utf-8') as fp:
                    self.gl_widget.set_frame_content(fp.read())
        except Exception as e:
            log(f"Error reading frame file: {e}")


def main():
    """Main entry point"""
    log("GL Renderer Service Started")
    log(f"Window size: {WINDOW_WIDTH}x{WINDOW_HEIGHT}")
    log("Reading from: pieces/display/current_frame.txt")
    log("Writing keyboard input to: pieces/keyboard/history.txt")
    log("Checking marker: pieces/master_ledger/frame_changed.txt")
    
    # Initialize GLUT (required for bitmap fonts)
    glutInit(sys.argv)
    
    # Create Qt application
    app = QApplication(sys.argv)
    
    # Create and show window
    window = GLRendererWindow()
    window.show()
    
    log("GL window opened")
    
    # Run event loop
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
