"""
Mini Window implementation for PyOS
Provides draggable, resizable windows for applications
"""

from PySide6.QtWidgets import QFrame, QLabel, QPushButton, QHBoxLayout, QVBoxLayout, QWidget, QSizePolicy
from PySide6.QtCore import Qt, QPoint
from PySide6.QtGui import QPainter, QBrush, QColor


class MiniWindow(QFrame):
    def __init__(self, title="Application", content_widget=None):
        super().__init__()
        
        # Window properties
        self.title = title
        self.drag_position = QPoint()
        
        # Set window frame style
        self.setFrameShape(QFrame.StyledPanel)
        self.setFrameShadow(QFrame.Raised)
        self.setLineWidth(2)
        
        # Set initial size and position - make it resizable
        self.resize(400, 300)  # Use resize instead of setFixedSize to make it resizable
        self.setMinimumSize(200, 150)  # Set minimum size
        self.setMaximumSize(10000, 10000)  # Set very large max size to allow resizing
        self.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)  # Allow expanding
        self.setWindowFlags(Qt.Widget)  # Make it a child widget rather than a separate window
        
        # Create title bar
        self.title_bar = QWidget()
        self.title_bar.setFixedHeight(30)
        self.title_bar.setStyleSheet("""
            QWidget {
                background-color: #4a4a4a;
                border-top-left-radius: 5px;
                border-top-right-radius: 5px;
            }
        """)
        
        title_layout = QHBoxLayout(self.title_bar)
        title_layout.setContentsMargins(5, 5, 5, 5)
        title_layout.setSpacing(5)
        
        # Title label
        self.title_label = QLabel(title)
        self.title_label.setStyleSheet("color: white; font-weight: bold;")
        title_layout.addWidget(self.title_label)
        
        # Control buttons
        self.minimize_button = QPushButton("_")
        self.minimize_button.setFixedSize(25, 25)
        self.minimize_button.clicked.connect(self.showMinimized)
        
        self.maximize_button = QPushButton("□")
        self.maximize_button.setFixedSize(25, 25)
        self.maximize_button.clicked.connect(self.toggle_maximize)
        
        self.close_button = QPushButton("X")
        self.close_button.setFixedSize(25, 25)
        self.close_button.clicked.connect(self.close)
        
        # Style the buttons
        button_style = """
            QPushButton {
                background-color: #6a6a6a;
                color: white;
                border: none;
                font-weight: bold;
            }
            QPushButton:hover {
                background-color: #7a7a7a;
            }
            QPushButton:pressed {
                background-color: #5a5a5a;
            }
        """
        self.minimize_button.setStyleSheet(button_style)
        self.maximize_button.setStyleSheet(button_style)
        self.close_button.setStyleSheet(button_style)
        
        title_layout.addWidget(self.minimize_button)
        title_layout.addWidget(self.maximize_button)
        title_layout.addWidget(self.close_button)
        
        # Create content area
        self.content_area = QWidget()
        self.content_area.setStyleSheet("background-color: #3a3a3a;")
        self.content_area.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        
        if content_widget:
            content_widget.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
            content_layout = QVBoxLayout(self.content_area)
            content_layout.setContentsMargins(0, 0, 0, 0)  # Remove margins to maximize space
            content_layout.addWidget(content_widget)
        else:
            # Default content
            content_layout = QVBoxLayout(self.content_area)
            default_label = QLabel("Application Content Area")
            default_label.setAlignment(Qt.AlignCenter)
            default_label.setStyleSheet("color: #ccc;")
            content_layout.addWidget(default_label)
        
        # Main layout
        main_layout = QVBoxLayout(self)
        main_layout.setContentsMargins(0, 0, 0, 0)
        main_layout.setSpacing(0)
        main_layout.addWidget(self.title_bar)
        main_layout.addWidget(self.content_area)
        
        # Enable mouse tracking to detect edge resizing
        self.setMouseTracking(True)
        
        # Install event filter for title bar to handle dragging
        self.title_bar.installEventFilter(self)
        
        # State variables for resizing and dragging
        self.resizing = False
        self.dragging = False
        self.resize_direction = None  # 'east', 'south', 'southeast', etc.
        self.is_near_edge = False
        

    
    def toggle_maximize(self):
        """Toggle window maximization"""
        # Since we're in a desktop environment, maximization might need special handling
        # For now, we'll just resize to fill available space within parent
        parent = self.parent()
        if parent:
            if hasattr(self, '_was_maximized') and self._was_maximized:
                # Restore to previous size - use resize instead of setFixedSize to maintain resizability
                self.resize(400, 300)
                self._was_maximized = False
                self.maximize_button.setText("□")
            else:
                # Maximize to parent size - use resize instead of setFixedSize to maintain resizability
                parent_size = parent.size()
                self.resize(parent_size.width() - 20, parent_size.height() - 20)
                self._was_maximized = True
                self.maximize_button.setText("❐")
    
    def setTitle(self, title):
        """Set the window title"""
        self.title = title
        self.title_label.setText(title)
        

        
    def eventFilter(self, obj, event):
        """Event filter to handle title bar dragging"""
        if obj == self.title_bar and event.type() == event.Type.MouseButtonPress:
            if event.button() == Qt.LeftButton:
                self.dragging = True
                self.drag_position = event.globalPos() - self.pos()
                event.accept()
                return True  # Event handled
        elif obj == self.title_bar and event.type() == event.Type.MouseMove:
            if self.dragging and event.buttons() == Qt.LeftButton:
                # Calculate new position
                new_pos = event.globalPos() - self.drag_position
                
                # Get parent's geometry to constrain movement (if parent exists)
                parent = self.parent()
                if parent:
                    parent_rect = parent.rect()
                    
                    # Keep window within parent boundaries
                    x = max(0, min(new_pos.x(), parent_rect.width() - self.width()))
                    y = max(0, min(new_pos.y(), parent_rect.height() - self.height()))
                    
                    self.move(x, y)
                else:
                    self.move(new_pos)
                
                event.accept()
                return True  # Event handled
        elif obj == self.title_bar and event.type() == event.Type.MouseButtonRelease:
            if event.button() == Qt.LeftButton:
                self.dragging = False
                event.accept()
                return True  # Event handled

        return super().eventFilter(obj, event)  # Let parent handle other events

    def mousePressEvent(self, event):
        """Handle mouse press for border resizing"""
        if event.button() == Qt.LeftButton:
            # Determine if click is near edge for resizing
            pos = event.pos()
            rect = self.rect()
            
            # Define resize zones (10 pixels from edge)
            resize_zone_size = 10
            
            # Check if near right edge
            if pos.x() >= rect.right() - resize_zone_size:
                if pos.y() >= rect.bottom() - resize_zone_size:
                    self.resize_direction = 'southeast'
                else:
                    self.resize_direction = 'east'
            elif pos.y() >= rect.bottom() - resize_zone_size:
                self.resize_direction = 'south'
            else:
                # Not near edge, don't initiate resizing
                self.resize_direction = None
                event.ignore()
                return  # Let other handlers process if needed
                
            self.resizing = True
            self.last_resize_pos = event.globalPos()
            event.accept()
        
    def mouseMoveEvent(self, event):
        """Handle mouse move for dragging or resizing"""
        pos = event.pos()
        rect = self.rect()
        resize_zone_size = 10
        
        # Change cursor based on position near edges
        if pos.x() >= rect.right() - resize_zone_size:
            if pos.y() >= rect.bottom() - resize_zone_size:
                self.setCursor(Qt.SizeFDiagCursor)  # Southeast diagonal resize
            else:
                self.setCursor(Qt.SizeHorCursor)  # Horizontal resize
            self.is_near_edge = True
        elif pos.y() >= rect.bottom() - resize_zone_size:
            self.setCursor(Qt.SizeVerCursor)  # Vertical resize
            self.is_near_edge = True
        else:
            self.is_near_edge = False
            # Only unset cursor if we're not currently resizing
            if not self.resizing:
                self.unsetCursor()
        
        # Handle actual resizing if in progress
        if self.resizing and self.resize_direction:
            current_pos = event.globalPos()
            delta = current_pos - self.last_resize_pos
            
            new_width = self.width()
            new_height = self.height()
            
            if 'east' in self.resize_direction:
                new_width = max(self.minimumWidth(), self.width() + delta.x())
            if 'south' in self.resize_direction:
                new_height = max(self.minimumHeight(), self.height() + delta.y())
                
            self.resize(new_width, new_height)
            self.last_resize_pos = current_pos
            event.accept()
        else:
            # Let the parent handle the event if not resizing
            super().mouseMoveEvent(event)
        
    def mouseReleaseEvent(self, event):
        """Handle mouse release"""
        if event.button() == Qt.LeftButton:
            self.resizing = False
            self.resize_direction = None
        event.accept()
        
    def leaveEvent(self, event):
        """Reset cursor when leaving window"""
        if not self.resizing:  # Don't reset if currently resizing
            self.unsetCursor()
        super().leaveEvent(event)