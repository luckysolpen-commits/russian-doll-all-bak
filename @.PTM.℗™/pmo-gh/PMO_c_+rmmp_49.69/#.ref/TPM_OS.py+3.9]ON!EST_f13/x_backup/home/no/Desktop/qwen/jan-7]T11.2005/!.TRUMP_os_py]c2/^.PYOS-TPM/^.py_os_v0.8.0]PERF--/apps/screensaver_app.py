"""
Simple OpenGL screensaver app for PyOS
This app serves as an OpenGL context initializer to prevent first-run issues
"""

from PySide6.QtWidgets import QWidget, QVBoxLayout, QLabel
from PySide6.QtCore import Qt
from graphics.cube_screensaver import CubeScreensaver


class ScreensaverApp(QWidget):
    def __init__(self):
        super().__init__()
        self.init_ui()
        
    def init_ui(self):
        layout = QVBoxLayout()
        
        # Title label
        title_label = QLabel("OpenGL Screensaver")
        title_label.setAlignment(Qt.AlignCenter)
        title_label.setStyleSheet("font-size: 18px; font-weight: bold; margin: 10px;")
        layout.addWidget(title_label)
        
        # Add the cube screensaver widget
        self.screensaver = CubeScreensaver()
        layout.addWidget(self.screensaver)
        
        # Add description
        desc_label = QLabel("Simple rotating cube screensaver for OpenGL context initialization")
        desc_label.setAlignment(Qt.AlignCenter)
        desc_label.setWordWrap(True)
        desc_label.setStyleSheet("margin: 10px;")
        layout.addWidget(desc_label)
        
        self.setLayout(layout)
        self.setWindowTitle("OpenGL Screensaver")
        
    def showEvent(self, event):
        """Called when the widget is shown"""
        super().showEvent(event)
        # Ensure the OpenGL context is active when showing
        if hasattr(self.screensaver, 'makeCurrent'):
            try:
                self.screensaver.makeCurrent()
            except:
                pass  # May not be needed depending on implementation
                
    def closeEvent(self, event):
        """Clean up when closing"""
        # Clean up OpenGL resources if needed
        event.accept()