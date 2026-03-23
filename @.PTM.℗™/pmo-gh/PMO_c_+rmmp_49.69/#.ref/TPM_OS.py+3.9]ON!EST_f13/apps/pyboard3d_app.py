"""
3D PyBoard Application for PyOS
A 3D voxel viewer application adapted from the 3D PyBoard staging code
"""

from PySide6.QtWidgets import QVBoxLayout, QHBoxLayout, QWidget, QLabel, QPushButton
from PySide6.QtCore import Qt
from PySide6.QtGui import QFont, QColor

from apps.base_app import BaseApplication
from PySide6.QtWidgets import QSizePolicy


class PyBoard3DApp(BaseApplication):
    def setup_ui(self):
        """Setup the 3D PyBoard application UI"""
        layout = QVBoxLayout(self)
        layout.setContentsMargins(10, 10, 10, 10)
        
        # Title
        title_label = QLabel("3D PyBoard Viewer")
        title_label.setStyleSheet("font-size: 18px; font-weight: bold; color: #ddd;")
        layout.addWidget(title_label)
        
        # Description
        desc_label = QLabel("A 3D voxel viewer with Z-level navigation")
        desc_label.setStyleSheet("color: #aaa; margin-top: 5px;")
        layout.addWidget(desc_label)
        
        # Import the 3D grid widget and minimap
        from apps.grid_widget_3d import GridWidget3D
        from apps.minimap_widget import MinimapWidget
        
        # Create the main 3D grid widget with minimap callback
        self.grid_widget_3d = GridWidget3D(
            grid_size=12, 
            cell_size=1.0, 
            parent=self,
            minimap_callback=self._update_minimap_from_3d
        )
        self.grid_widget_3d.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        
        # Create the minimap widget as an overlay (child of the 3D widget)
        self.minimap_widget = MinimapWidget(self.grid_widget_3d)
        self.minimap_widget.setParent(self.grid_widget_3d)  # Make it an overlay
        self.minimap_widget.setVisible(True)  # Show minimap
        
        # Position the minimap in the top-right corner of the 3D view
        self.grid_widget_3d.minimap_widget = self.minimap_widget  # Provide reference to 3D widget
        
        # Create a central widget to contain the 3D view in a way that expands properly
        self.central_widget = QWidget()
        central_layout = QVBoxLayout(self.central_widget)
        central_layout.setContentsMargins(0, 0, 0, 0)
        
        # Add the 3D view to the central layout to maximize space
        central_layout.addWidget(self.grid_widget_3d)
        self.central_widget.setLayout(central_layout)
        
        # Add the central widget to the main layout
        layout.addWidget(self.central_widget, 1)  # Give it full expansion priority
        
        # Give the widget focus to receive keyboard events
        self.grid_widget_3d.setFocusPolicy(Qt.StrongFocus)  # Ensure it can receive focus
        self.grid_widget_3d.setFocus()
        
        # Force initial show event to initialize the 3D content
        # This triggers the initialization that happens in showEvent
        self.grid_widget_3d.setVisible(True)
    
    def _update_minimap_from_3d(self, entities, selector_pos, pov_mode, camera_direction):
        """Update the minimap from 3D view data"""
        # Update the minimap with current state
        self.minimap_widget.update_data(
            entities,
            selector_pos,
            pov_mode,
            camera_direction
        )