#!/usr/bin/env python3
"""
GL Renderer - Qt/OpenGL window display
Standardized for PMO v1.4.0 (Single Pulse Standard).
Reads from pieces/display/current_frame.txt.
Monitors pieces/display/frame_changed.txt for updates.
"""

import os
import sys
import time

# Import TPM Utils
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '../../locations'))
import path_utils

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
    print(f"GL Import Error: {e}", file=sys.stderr)
    sys.exit(1)

# Constants
WINDOW_WIDTH = 800
WINDOW_HEIGHT = 1200
BACKGROUND_COLOR = (0.1, 0.1, 0.1, 1.0)
TEXT_COLOR = (1.0, 1.0, 1.0, 1.0)

class RenderGLWidget(QOpenGLWidget):
    def __init__(self):
        super().__init__()
        self.frame_content = ""
        self.lines = []
        self.setMinimumSize(WINDOW_WIDTH, WINDOW_HEIGHT)
        self.setFocusPolicy(Qt.StrongFocus)
    
    def set_frame_content(self, content):
        self.frame_content = content
        self.lines = content.split('\n')
        self.update()
    
    def initializeGL(self):
        glClearColor(*BACKGROUND_COLOR)
        glEnable(GL_BLEND)
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
    
    def resizeGL(self, w, h):
        glViewport(0, 0, w, h)
        glMatrixMode(GL_PROJECTION)
        glLoadIdentity()
        glOrtho(0, w, h, 0, -1, 1)
        glMatrixMode(GL_MODELVIEW)
        glLoadIdentity()
    
    def paintGL(self):
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
        glLoadIdentity()
        glColor3f(*TEXT_COLOR[:3])
        y_position = 50
        line_spacing = 22
        for line in self.lines:
            self.render_text(line, 50, y_position)
            y_position += line_spacing
    
    def render_text(self, text, x, y):
        glRasterPos2f(x, y)
        for char in text:
            try: glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, ord(char))
            except: pass

    def keyPressEvent(self, event):
        key = event.key()
        key_code = 0
        if key == Qt.Key_Up: key_code = 1002
        elif key == Qt.Key_Down: key_code = 1003
        elif key == Qt.Key_Left: key_code = 1000
        elif key == Qt.Key_Right: key_code = 1001
        elif key == Qt.Key_W: key_code = ord('w')
        elif key == Qt.Key_S: key_code = ord('s')
        elif key == Qt.Key_A: key_code = ord('a')
        elif key == Qt.Key_D: key_code = ord('d')
        elif key == Qt.Key_Return or key == Qt.Key_Enter: key_code = 13
        elif key == Qt.Key_Escape: key_code = 27
        elif Qt.Key_0 <= key <= Qt.Key_9: key_code = ord('0') + (key - Qt.Key_0)
        elif Qt.Key_A <= key <= Qt.Key_Z: key_code = ord('a') + (key - Qt.Key_A)
        
        if key_code > 0:
            hist_path = path_utils.get_piece_path('keyboard', 'history.txt')
            if hist_path:
                with open(hist_path, 'a') as f: f.write(f"{key_code}\n")

class GLRendererWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        path_utils.init_paths()
        self.last_pulse_size = 0
        self.gl_widget = RenderGLWidget()
        self.setCentralWidget(self.gl_widget)
        self.setWindowTitle("TPM FuzzPet - GL Sovereign Renderer")
        self.setGeometry(100, 100, WINDOW_WIDTH, WINDOW_HEIGHT)
        
        self.timer = QTimer()
        self.timer.timeout.connect(self.check_for_updates)
        self.timer.start(16) # ~60 FPS
        
        pulse_path = path_utils.get_piece_path('display', 'frame_changed.txt')
        if pulse_path and os.path.exists(pulse_path):
            self.last_pulse_size = os.path.getsize(pulse_path)
        self.load_frame()

    def check_for_updates(self):
        pulse_path = path_utils.get_piece_path('display', 'frame_changed.txt')
        if pulse_path and os.path.exists(pulse_path):
            curr_size = os.path.getsize(pulse_path)
            if curr_size > self.last_pulse_size:
                self.load_frame()
                self.last_pulse_size = curr_size

    def load_frame(self):
        frame_path = path_utils.get_piece_path('display', 'current_frame.txt')
        if frame_path and os.path.exists(frame_path):
            with open(frame_path, 'r', encoding='utf-8') as f:
                self.gl_widget.set_frame_content(f.read())

def main():
    glutInit(sys.argv)
    app = QApplication(sys.argv)
    window = GLRendererWindow()
    window.show()
    sys.exit(app.exec())

if __name__ == "__main__":
    main()
