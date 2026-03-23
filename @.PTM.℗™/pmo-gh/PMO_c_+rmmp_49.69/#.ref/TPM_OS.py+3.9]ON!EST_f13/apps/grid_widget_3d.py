"""
3D Grid Widget for PyOS - extracted from voxel_viewer_3d3z_local.py
This contains the core 3D OpenGL rendering and interaction code
"""

import sys
import math
import csv
import os
import numpy as np  # Import numpy for efficient array handling
from PySide6.QtWidgets import QApplication, QMainWindow, QLabel, QWidget
from PySide6.QtCore import Qt, QTimer, QThread, Signal
from PySide6.QtGui import QPainter, QColor, QFont, QPen
from PySide6.QtOpenGLWidgets import QOpenGLWidget
from OpenGL.GL import *
from OpenGL.GLU import *


class DataLoaderThread(QThread):
    """Thread to load data without blocking the UI"""
    data_loaded = Signal(object)  # Signal to emit loaded data
    
    def __init__(self, grid_size, cell_size):
        super().__init__()
        self.grid_size = grid_size
        self.cell_size = cell_size
    
    def run(self):
        """Load data in a separate thread"""
        entities = self.load_data_internal()
        self.data_loaded.emit(entities)
    
    def load_data_internal(self):
        """Internal method to load data"""
        entities = {}
        # Look for data in the shared pyboards_data directory within apps
        import os
        script_dir = os.path.dirname(os.path.abspath(__file__))  # Current apps directory
        file_path = os.path.join(script_dir, 'pyboards_data', 'aggregated_data.csv')
        try:
            # Try to import pandas in the thread
            import pandas as pd
            df = pd.read_csv(file_path)
            attributes_df = df[(df['source_file'].str.contains('entity_attributes.csv', na=False)) & (df['entity_id'].isin(['adam', 'eve']))]
            for entity_id in ['adam', 'eve']:
                entity_attrs = attributes_df[(attributes_df['entity_id'] == entity_id) & (attributes_df['attribute'].isin(['x', 'z']))]
                if not entity_attrs.empty:
                    x = float(entity_attrs[entity_attrs['attribute'] == 'x']['value'].iloc[0])
                    z = float(entity_attrs[entity_attrs['attribute'] == 'z']['value'].iloc[0])
                    pos_key = f"{int(z)},{int(x)}"  # Using z as layer, x as column
                    
                    # Load 3D voxel model for the entity
                    voxel_model = self.load_voxel_model_internal(entity_id)
                    
                    entities[pos_key] = {
                        'color': 'red' if entity_id == 'adam' else 'blue', 
                        'z': int(z), 
                        'x': int(x), 
                        'y': int(z),  # Using same value for y to keep at appropriate level
                        'id': entity_id, 
                        'voxel_model': voxel_model
                    }
        except FileNotFoundError:
            print("aggregated_data.csv not found. Please generate it first.")
            # Add example entities if file not found
            entities["1,2"] = {'color': 'red', 'z': 1, 'x': 2, 'y': 1, 'id': 'adam', 'voxel_model': self.load_voxel_model_internal('adam')}
            entities["3,4"] = {'color': 'blue', 'z': 3, 'x': 4, 'y': 3, 'id': 'eve', 'voxel_model': self.load_voxel_model_internal('eve')}
        except Exception as e:
            print(f"Error loading data: {e}")
            # Add example entities if error occurs
            entities["1,2"] = {'color': 'red', 'z': 1, 'x': 2, 'y': 1, 'id': 'adam', 'voxel_model': self.load_voxel_model_internal('adam')}
            entities["3,4"] = {'color': 'blue', 'z': 3, 'x': 4, 'y': 3, 'id': 'eve', 'voxel_model': self.load_voxel_model_internal('eve')}
        
        return entities

    def load_voxel_model_internal(self, entity_id):
        """Internal method to load voxel model (thread-safe)"""
        import csv
        import os
        script_dir = os.path.dirname(os.path.abspath(__file__))  # Current apps directory
        entity_dir = os.path.join(script_dir, 'pyboards_data', 'entities', entity_id, 'voxel_8x8x8')
        
        raw_voxel_data = []
        try:
            # Look for CSV files in the entity's voxel_8x8x8 directory
            for file_name in os.listdir(entity_dir):
                if file_name.endswith('.csv'):
                    csv_path = os.path.join(entity_dir, file_name)
                    with open(csv_path, 'r') as f:
                        reader = csv.DictReader(f)
                        for row in reader:
                            x = float(row['x'])
                            y = float(row['y'])
                            z = float(row['z'])
                            r = int(row['r']) / 255.0
                            g = int(row['g']) / 255.0
                            b = int(row['b']) / 255.0
                            # Skip black voxels (r=0, g=0, b=0) as they are considered transparent
                            if not (r == 0.0 and g == 0.0 and b == 0.0):
                                raw_voxel_data.append((x, y, z, r, g, b))
                    break  # Use the first CSV file found
        except FileNotFoundError:
            print(f"Voxel model CSV not found for {entity_id}. The entity will only show the foundation.")
            # Return empty list if no voxel data is found
        except Exception as e:
            print(f"Error loading voxel model for {entity_id}: {e}")
            # Return empty list if there's an error
        
        # Normalize coordinates to fit within 0-7 range for an 8x8x8 model
        if not raw_voxel_data:
            return raw_voxel_data
            
        # Find min and max for each axis
        min_x = min(v[0] for v in raw_voxel_data)
        max_x = max(v[0] for v in raw_voxel_data)
        min_y = min(v[1] for v in raw_voxel_data)
        max_y = max(v[1] for v in raw_voxel_data)
        min_z = min(v[2] for v in raw_voxel_data)
        max_z = max(v[2] for v in raw_voxel_data)
        
        # Calculate scale factors to fit within 0-7 range
        size_x = max_x - min_x if max_x != min_x else 1
        size_y = max_y - min_y if max_y != min_y else 1
        size_z = max_z - min_z if max_z != min_z else 1
        
        # Normalize the coordinates to 0-7 range
        normalized_voxel_data = []
        for x, y, z, r, g, b in raw_voxel_data:
            # Map from original range to 0-7 range
            norm_x = 7.0 * (x - min_x) / size_x
            norm_y = 7.0 * (y - min_y) / size_y
            norm_z = 7.0 * (z - min_z) / size_z
            # Ensure values are within bounds
            norm_x = max(0, min(7, norm_x))
            norm_y = max(0, min(7, norm_y))
            norm_z = max(0, min(7, norm_z))
            normalized_voxel_data.append((norm_x, norm_y, norm_z, r, g, b))
        
        return normalized_voxel_data


class UIElement:
    """Base class for UI elements in the 3D view"""
    def __init__(self, x=0, y=0, width=0, height=0, z_order=0, element_id=None):
        self.x = x
        self.y = y
        self.width = width
        self.height = height
        self.z_order = z_order  # Higher values appear on top
        self.visible = True
        self.element_id = element_id  # Optional ID for identifying elements
    
    def draw(self, widget):
        """Override this method to draw the UI element"""
        pass


class TranslucentRect(UIElement):
    """Translucent rectangle UI element"""
    def __init__(self, x=0, y=0, width=0, height=0, opacity=0.5, color=(0.0, 0.0, 0.0), element_id=None, border_enabled=True, border_color=(1.0, 1.0, 1.0)):
        super().__init__(x, y, width, height, element_id=element_id)
        self.opacity = opacity
        self.color = color  # RGB tuple
        self.border_enabled = border_enabled  # Whether to draw border
        self.border_color = border_color  # Color of the border (RGB)
    
    def draw(self, widget):
        """Draw the translucent rectangle using OpenGL"""
        # First draw the translucent background
        widget.draw_translucent_rect_opengl(self.x, self.y, self.width, self.height, self.opacity, self.color)
        
        # Then draw the border if enabled
        if self.border_enabled:
            widget.draw_rect_border_opengl(self.x, self.y, self.width, self.height, self.border_color)


class TextElement(UIElement):
    """Text UI element that is rendered by the parent widget"""
    def __init__(self, x=0, y=0, text="", font_size=10, color=QColor(255, 255, 255), element_id=None):
        # Width and height will be calculated based on text
        super().__init__(x, y, 0, 0, element_id=element_id)
        self.text = text
        self.font_size = font_size
        self.color = color
        self.font = QFont("Arial", font_size, QFont.Bold)
    
    def draw(self, widget):
        """Text elements are drawn collectively by the parent widget"""
        # This method is only here to satisfy the interface
        # Actual drawing is handled in _draw_ui_elements
        pass


class GridWidget3D(QOpenGLWidget):
    def __init__(self, grid_size, cell_size, parent=None, minimap_callback=None):
        super().__init__()
        
        self.parent_window = parent  # Store reference to parent window
        self.minimap_callback = minimap_callback  # Callback for minimap updates
        self.grid_size = grid_size
        self.cell_size = cell_size
        self.entities = {}
        self.selector_pos = [0, 0, 0]  # Starting position of selector [z, y, x]
        
        # Toggle for entity rotation: True to rotate entities 90° around X-axis, False for original orientation
        self.rotate_entities = True
        self.is_selected = False
        self.selected_entity_key_when_picked_up = None # The key of the cell the entity was removed from
        self.selected_entity_data = None # The entity data itself, removed from self.entities
        
        # Camera controls
        self.camera_x = 0.0
        self.camera_y = -12.0
        self.camera_z = 12.0
        self.camera_angle_x = -45  # Looking down at a moderate angle
        self.camera_angle_y = 0  # Looking straight (no rotation)
        
        # Movement speed
        self.move_speed = 0.5

        # POV modes: 0=third_person, 1=first_person, 2=free_camera
        self.pov_mode = 2  # Start in free camera mode
        self.pov_modes = ["Third Person", "First Person", "Free Camera"]
        
        # Store selector position for camera following in first/third person modes
        self.last_selector_pos = [0, 0, 0]
        
        # Camera direction for first person POV (in radians)
        self.camera_direction = 0  # Start facing +Y direction (with -π/2 offset in update_camera gives -π/2 actual rotation -> +Y)

        self.setFocusPolicy(Qt.StrongFocus)
        
        # Initialize UI elements
        self.ui_elements = []
        self._init_ui_elements()
        
        # VBO/VAO attributes for voxel rendering (when data is loaded)
        self.vao = 0
        self.vbo_vertices = 0
        self.vbo_colors = 0
        self.voxel_count = 0
        self.initialized_gl_resources = False # Flag to track GL resource initialization
        self.entity_data_hash = None  # Track changes to entity data to know when to rebuild VBOs
        
        # VBO/VAO attributes for grid rendering
        self.grid_vao = 0
        self.grid_vbo_vertices = 0
        self.grid_line_count = 0
        
        # State for resize performance
        self.resizing = False
        # Timer for managing resize state
        self.resize_timer = QTimer()
        self.resize_timer.setSingleShot(True)
        self.resize_timer.timeout.connect(self._end_resize_operation)
        
        # Defer initialization until widget is shown and use async loading
        self.needs_init = True
        self.initialized = False
        self.loading_complete = False
        self.data_loaded = False
        self.entities_data = {}  # Store entities data separately to avoid blocking on large data
        self.minimap_widget = None  # Reference to the minimap widget (will be set from PyBoard3DApp)
    
    def showEvent(self, event):
        """Initialize entities when widget is first shown"""
        super().showEvent(event)
        if self.needs_init:
            # Start initialization after a small delay to allow UI to show
            from PySide6.QtCore import QTimer
            QTimer.singleShot(100, self.init_entities_async)  # Delay init by 100ms
            self.needs_init = False
    
    def init_entities_async(self):
        """Initialize entities asynchronously using a worker thread"""
        # Create and start the data loader thread
        self.data_loader_thread = DataLoaderThread(self.grid_size, self.cell_size)
        self.data_loader_thread.data_loaded.connect(self.on_data_loaded)
        self.data_loader_thread.start()
    
    def on_data_loaded(self, entities_data):
        """Called when data loading is complete"""
        # Update the entities in the main thread
        self.entities = entities_data
        self.data_loaded = True
        self.loading_complete = True
        # Update UI elements with new data
        self._update_ui_elements()
        # Setup VBO/VAO for voxel rendering since data is now available
        self._setup_voxel_buffers()
        # Trigger a redraw to show the entities
        self.update()
    
    def _init_ui_elements(self):
        """Initialize UI elements for the 3D view"""
        # Add a yellow rectangle as proof of concept
        yellow_rect = TranslucentRect(200, 200, 150, 100, opacity=0.7, color=(0.8, 0.8, 0.0),  # Yellow translucent
                                   border_enabled=True, border_color=(1.0, 1.0, 0.0))  # Bright yellow border
        yellow_rect.element_id = "proof_rect"  # Unique identifier
        yellow_rect.z_order = 10  # Higher than background elements
        self.ui_elements.append(yellow_rect)
        
        # Add selector position readout
        pos_rect = TranslucentRect(10, 45, 250, 25, 0.4, 
                                border_enabled=True, border_color=(1.0, 1.0, 0.0))  # Yellow border
        pos_rect.z_order = 15
        self.ui_elements.append(pos_rect)
        
        # Position text element
        pos_text = f"Selector X,Y,Z: {self.selector_pos[2]},{self.selector_pos[1]},{self.selector_pos[0]}"
        pos_text_elem = TextElement(15, 63, pos_text, 12, QColor(0, 255, 255))
        pos_text_elem.z_order = 16
        self.ui_elements.append(pos_text_elem)
        
        # Add POV mode indicator
        pov_rect = TranslucentRect(10, 75, 200, 25, 0.4,
                                border_enabled=True, border_color=(1.0, 1.0, 0.0))  # Yellow border
        pov_rect.z_order = 15
        self.ui_elements.append(pov_rect)
        
        pov_text_elem = TextElement(15, 93, f"POV Mode: {self.pov_modes[self.pov_mode]}", 12, QColor(255, 255, 0))
        pov_text_elem.z_order = 16
        self.ui_elements.append(pov_text_elem)
        
        # Add controls text
        controls_rect = TranslucentRect(10, 105, 250, 60, 0.4,
                                     border_enabled=True, border_color=(1.0, 1.0, 0.0))  # Yellow border
        controls_rect.z_order = 15
        self.ui_elements.append(controls_rect)
        
        controls_text_elem1 = TextElement(15, 120, "WASD: Move, Arrow Keys: Move Selector", 9, QColor(200, 200, 200))
        controls_text_elem1.z_order = 16
        self.ui_elements.append(controls_text_elem1)
        
        controls_text_elem2 = TextElement(15, 135, "Q/E: Turn Left/Right, Z/X: Change Z-Level", 9, QColor(200, 200, 200))
        controls_text_elem2.z_order = 16
        self.ui_elements.append(controls_text_elem2)
        
        controls_text_elem3 = TextElement(15, 150, "0: Cycle POV, F: Toggle Rotation", 9, QColor(200, 200, 200))
        controls_text_elem3.z_order = 16
        self.ui_elements.append(controls_text_elem3)

    def init_entities(self):
        """Initialize the entities on the grid"""
        # Clear existing entities
        self.entities = {}
        
        # Load data to determine entity positions
        self.load_data()
        
    def load_data(self):
        """Load entity data from CSV"""
        # Look for data in the shared pyboards_data directory within apps
        script_dir = os.path.dirname(os.path.abspath(__file__))  # Current apps directory
        file_path = os.path.join(script_dir, 'pyboards_data', 'aggregated_data.csv')
        try:
            import pandas as pd
            df = pd.read_csv(file_path)
            attributes_df = df[(df['source_file'].str.contains('entity_attributes.csv', na=False)) & (df['entity_id'].isin(['adam', 'eve']))]
            for entity_id in ['adam', 'eve']:
                entity_attrs = attributes_df[(attributes_df['entity_id'] == entity_id) & (attributes_df['attribute'].isin(['x', 'z']))]
                if not entity_attrs.empty:
                    x = float(entity_attrs[entity_attrs['attribute'] == 'x']['value'].iloc[0])
                    z = float(entity_attrs[entity_attrs['attribute'] == 'z']['value'].iloc[0])
                    pos_key = f"{int(z)},{int(x)}"  # Using z as layer, x as column
                    
                    # Load 3D voxel model for the entity
                    voxel_model = self.load_voxel_model(entity_id)
                    
                    self.entities[pos_key] = {
                        'color': 'red' if entity_id == 'adam' else 'blue', 
                        'z': int(z), 
                        'x': int(x), 
                        'y': int(z),  # Using same value for y to keep at appropriate level
                        'id': entity_id, 
                        'voxel_model': voxel_model
                    }
        except FileNotFoundError:
            print("aggregated_data.csv not found. Please generate it first.")
            # Add example entities if file not found
            self.entities["1,2"] = {'color': 'red', 'z': 1, 'x': 2, 'y': 1, 'id': 'adam', 'voxel_model': self.load_voxel_model('adam')}
            self.entities["3,4"] = {'color': 'blue', 'z': 3, 'x': 4, 'y': 3, 'id': 'eve', 'voxel_model': self.load_voxel_model('eve')}
        except Exception as e:
            print(f"Error loading data: {e}")
            # Add example entities if error occurs
            self.entities["1,2"] = {'color': 'red', 'z': 1, 'x': 2, 'y': 1, 'id': 'adam', 'voxel_model': self.load_voxel_model('adam')}
            self.entities["3,4"] = {'color': 'blue', 'z': 3, 'x': 4, 'y': 3, 'id': 'eve', 'voxel_model': self.load_voxel_model('eve')}

    def load_voxel_model(self, entity_id):
        """Load the 8x8x8 voxel CSV file for an entity from the voxel_8x8x8 directory and normalize coordinates"""
        script_dir = os.path.dirname(os.path.abspath(__file__))  # Current apps directory
        entity_dir = os.path.join(script_dir, 'pyboards_data', 'entities', entity_id, 'voxel_8x8x8')
        
        raw_voxel_data = []
        try:
            # Look for CSV files in the entity's voxel_8x8x8 directory
            for file_name in os.listdir(entity_dir):
                if file_name.endswith('.csv'):
                    csv_path = os.path.join(entity_dir, file_name)
                    with open(csv_path, 'r') as f:
                        reader = csv.DictReader(f)
                        for row in reader:
                            x = float(row['x'])
                            y = float(row['y'])
                            z = float(row['z'])
                            r = int(row['r']) / 255.0
                            g = int(row['g']) / 255.0
                            b = int(row['b']) / 255.0
                            # Skip black voxels (r=0, g=0, b=0) as they are considered transparent
                            if not (r == 0.0 and g == 0.0 and b == 0.0):
                                raw_voxel_data.append((x, y, z, r, g, b))
                    break  # Use the first CSV file found
        except FileNotFoundError:
            print(f"Voxel model CSV not found for {entity_id}. The entity will only show the foundation.")
            # Return empty list if no voxel data is found
        except Exception as e:
            print(f"Error loading voxel model for {entity_id}: {e}")
            # Return empty list if there's an error
        
        # Normalize coordinates to fit within 0-7 range for an 8x8x8 model
        if not raw_voxel_data:
            return raw_voxel_data
            
        # Find min and max for each axis
        min_x = min(v[0] for v in raw_voxel_data)
        max_x = max(v[0] for v in raw_voxel_data)
        min_y = min(v[1] for v in raw_voxel_data)
        max_y = max(v[1] for v in raw_voxel_data)
        min_z = min(v[2] for v in raw_voxel_data)
        max_z = max(v[2] for v in raw_voxel_data)
        
        # Calculate scale factors to fit within 0-7 range
        size_x = max_x - min_x if max_x != min_x else 1
        size_y = max_y - min_y if max_y != min_y else 1
        size_z = max_z - min_z if max_z != min_z else 1
        
        # Normalize the coordinates to 0-7 range
        normalized_voxel_data = []
        for x, y, z, r, g, b in raw_voxel_data:
            # Map from original range to 0-7 range
            norm_x = 7.0 * (x - min_x) / size_x
            norm_y = 7.0 * (y - min_y) / size_y
            norm_z = 7.0 * (z - min_z) / size_z
            # Ensure values are within bounds
            norm_x = max(0, min(7, norm_x))
            norm_y = max(0, min(7, norm_y))
            norm_z = max(0, min(7, norm_z))
            normalized_voxel_data.append((norm_x, norm_y, norm_z, r, g, b))
        
        return normalized_voxel_data

    def _setup_voxel_buffers(self):
        """Generates voxel geometry and uploads it to VBOs/VAOs"""
        # Ensure we have OpenGL context current
        # This method should be called only after context is available and data loaded
        try:
            self.makeCurrent()
        except:
            # If context not available yet, skip for now
            return

        # Clear any potential OpenGL errors before proceeding
        while glGetError() != GL_NO_ERROR:
            pass

        # Define cube vertices for each face (triangles: 2 triangles per face = 6 vertices per face)
        # Format: [x, y, z, x, y, z, ...] for each triangle
        # Each face of the cube has 2 triangles (6 vertices total per face)
        face_vertices = np.array([
            # Back face (-Z) - vertices 0,1,2 and 0,2,3
            -0.5, -0.5, -0.5,   # Vertex 0
             0.5, -0.5, -0.5,   # Vertex 1
             0.5,  0.5, -0.5,   # Vertex 2
            -0.5, -0.5, -0.5,   # Vertex 0 again
             0.5,  0.5, -0.5,   # Vertex 2 again
            -0.5,  0.5, -0.5,   # Vertex 3
            
            # Front face (+Z) - vertices 4,5,6 and 4,6,7
            -0.5, -0.5,  0.5,   # Vertex 4
             0.5,  0.5,  0.5,   # Vertex 6
             0.5, -0.5,  0.5,   # Vertex 5
            -0.5, -0.5,  0.5,   # Vertex 4 again
            -0.5,  0.5,  0.5,   # Vertex 7
             0.5,  0.5,  0.5,   # Vertex 6 again
            
            # Left face (-X) - vertices 0,3,7 and 0,7,4
            -0.5, -0.5, -0.5,   # Vertex 0
            -0.5,  0.5, -0.5,   # Vertex 3
            -0.5,  0.5,  0.5,   # Vertex 7
            -0.5, -0.5, -0.5,   # Vertex 0 again
            -0.5,  0.5,  0.5,   # Vertex 7 again
            -0.5, -0.5,  0.5,   # Vertex 4
            
            # Right face (+X) - vertices 1,5,6 and 1,6,2
             0.5, -0.5, -0.5,   # Vertex 1
             0.5, -0.5,  0.5,   # Vertex 5
             0.5,  0.5,  0.5,   # Vertex 6
             0.5, -0.5, -0.5,   # Vertex 1 again
             0.5,  0.5,  0.5,   # Vertex 6 again
             0.5,  0.5, -0.5,   # Vertex 2
            
            # Top face (+Y) - vertices 3,2,6 and 3,6,7
            -0.5,  0.5, -0.5,   # Vertex 3
             0.5,  0.5, -0.5,   # Vertex 2
             0.5,  0.5,  0.5,   # Vertex 6
            -0.5,  0.5, -0.5,   # Vertex 3 again
             0.5,  0.5,  0.5,   # Vertex 6 again
            -0.5,  0.5,  0.5,   # Vertex 7
            
            # Bottom face (-Y) - vertices 0,4,5 and 0,5,1
            -0.5, -0.5, -0.5,   # Vertex 0
            -0.5, -0.5,  0.5,   # Vertex 4
             0.5, -0.5,  0.5,   # Vertex 5
            -0.5, -0.5, -0.5,   # Vertex 0 again
             0.5, -0.5,  0.5,   # Vertex 5 again
             0.5, -0.5, -0.5,   # Vertex 1
        ], dtype=np.float32)

        all_vertices = []
        all_colors = []
        
        # Generate data for entities
        for pos_key, entity in self.entities.items():
            base_x = entity['x'] * self.cell_size
            base_y = (self.grid_size - 1 - entity['y']) * self.cell_size
            base_z = entity['z'] * self.cell_size
            
            voxel_model = entity.get('voxel_model', [])
            
            if voxel_model:
                voxel_size = self.cell_size / 8.0
                
                for voxel in voxel_model:
                    if len(voxel) >= 6:
                        orig_x, orig_y, orig_z, r, g, b = voxel[:6]
                        
                        # Apply rotation if enabled
                        if self.rotate_entities:
                            rot_x = orig_x
                            rot_y = orig_z
                            rot_z = -orig_y
                            rot_z = 7 + rot_z
                        else:
                            rot_x = orig_x
                            rot_y = orig_y
                            rot_z = orig_z
                        
                        # Calculate center of the voxel in world coordinates
                        center_x = base_x + rot_x * voxel_size + voxel_size / 2
                        center_y = base_y + (7 - rot_y) * voxel_size + voxel_size / 2 # Invert Y for correct rendering
                        center_z = base_z + rot_z * voxel_size + voxel_size / 2
                        
                        # For each vertex of the cube template, apply translation to position it
                        for i in range(0, len(face_vertices), 3):
                            all_vertices.append(face_vertices[i] * voxel_size + center_x)
                            all_vertices.append(face_vertices[i+1] * voxel_size + center_y)
                            all_vertices.append(face_vertices[i+2] * voxel_size + center_z)
                            
                            # Assign same color to all vertices of this voxel
                            all_colors.extend([r, g, b])

        self.voxel_count = len(all_vertices) // 3 # Total number of vertices to draw
        
        if self.voxel_count > 0:
            # Convert to numpy arrays
            vertices_data = np.array(all_vertices, dtype=np.float32)
            colors_data = np.array(all_colors, dtype=np.float32)
            
            # Generate VAO if not already generated
            if self.vao == 0:
                self.vao = glGenVertexArrays(1)
            glBindVertexArray(self.vao)
            
            # Generate VBOs if not already generated
            if self.vbo_vertices == 0:
                self.vbo_vertices = glGenBuffers(1)
            glBindBuffer(GL_ARRAY_BUFFER, self.vbo_vertices)
            glBufferData(GL_ARRAY_BUFFER, vertices_data.nbytes, vertices_data, GL_STATIC_DRAW)
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, None)
            glEnableVertexAttribArray(0)
            
            if self.vbo_colors == 0:
                self.vbo_colors = glGenBuffers(1)
            glBindBuffer(GL_ARRAY_BUFFER, self.vbo_colors)
            glBufferData(GL_ARRAY_BUFFER, colors_data.nbytes, colors_data, GL_STATIC_DRAW)
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, None)
            glEnableVertexAttribArray(1)
            
            # Unbind VAO and VBOs
            glBindBuffer(GL_ARRAY_BUFFER, 0)
            glBindVertexArray(0)
            
        else:
            # No voxels to draw, ensure resources are cleared
            if self.vao != 0:
                glDeleteVertexArrays(1, [self.vao])
                self.vao = 0
            if self.vbo_vertices != 0:
                glDeleteBuffers(1, [self.vbo_vertices])
                self.vbo_vertices = 0
            if self.vbo_colors != 0:
                glDeleteBuffers(1, [self.vbo_colors])
                self.vbo_colors = 0
                
        # Check for OpenGL errors
        error = glGetError()
        if error != GL_NO_ERROR:
            print(f"[ERROR] OpenGL error after _setup_voxel_buffers: {error}")

    def _setup_grid_buffers(self):
        """Generates grid line geometry and uploads it to VBOs/VAOs"""
        # Ensure OpenGL context is current
        self.makeCurrent()
        
        # Clear any potential OpenGL errors before proceeding
        while glGetError() != GL_NO_ERROR:
            pass
            
        # Generate grid line vertices
        grid_lines = []
        
        # Draw grid lines in X-Y plane at each Z level for all layers
        for z in range(self.grid_size + 1):  # +1 to include the final boundary line
            # Lines along X axis (parallel to X)
            for i in range(self.grid_size + 1):
                # Line from (0, i*cell_size, z*cell_size) to (grid_size*cell_size, i*cell_size, z*cell_size)
                grid_lines.extend([0, i * self.cell_size, z * self.cell_size])
                grid_lines.extend([self.grid_size * self.cell_size, i * self.cell_size, z * self.cell_size])
                
            # Lines along Y axis (parallel to Y) 
            for i in range(self.grid_size + 1):
                # Line from (i*cell_size, 0, z*cell_size) to (i*cell_size, grid_size*cell_size, z*cell_size)
                grid_lines.extend([i * self.cell_size, 0, z * self.cell_size])
                grid_lines.extend([i * self.cell_size, self.grid_size * self.cell_size, z * self.cell_size])
        
        # Also add vertical lines connecting the layers
        for x in range(self.grid_size + 1):
            for y in range(self.grid_size + 1):
                # Line from (x*cell_size, y*cell_size, 0) to (x*cell_size, y*cell_size, grid_size*cell_size)
                grid_lines.extend([x * self.cell_size, y * self.cell_size, 0])
                grid_lines.extend([x * self.cell_size, y * self.cell_size, self.grid_size * self.cell_size])

        self.grid_line_count = len(grid_lines) // 3  # Total number of vertices
        
        if self.grid_line_count > 0:
            # Convert to numpy array
            vertices_data = np.array(grid_lines, dtype=np.float32)
            
            # Generate VAO if not already generated
            if self.grid_vao == 0:
                self.grid_vao = glGenVertexArrays(1)
            glBindVertexArray(self.grid_vao)
            
            # Generate VBO if not already generated
            if self.grid_vbo_vertices == 0:
                self.grid_vbo_vertices = glGenBuffers(1)
            glBindBuffer(GL_ARRAY_BUFFER, self.grid_vbo_vertices)
            glBufferData(GL_ARRAY_BUFFER, vertices_data.nbytes, vertices_data, GL_STATIC_DRAW)
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, None)
            glEnableVertexAttribArray(0)
            
            # Unbind VAO and VBO
            glBindBuffer(GL_ARRAY_BUFFER, 0)
            glBindVertexArray(0)
            
        else:
            # No grid lines to draw, ensure resources are cleared
            if self.grid_vao != 0:
                glDeleteVertexArrays(1, [self.grid_vao])
                self.grid_vao = 0
            if self.grid_vbo_vertices != 0:
                glDeleteBuffers(1, [self.grid_vbo_vertices])
                self.grid_vbo_vertices = 0
                
        # Check for OpenGL errors
        error = glGetError()
        if error != GL_NO_ERROR:
            print(f"[ERROR] OpenGL error after _setup_grid_buffers: {error}")

    def cleanupGL(self):
        """Clean up OpenGL resources when widget is destroyed"""
        # Ensure we have a valid OpenGL context
        try:
            self.makeCurrent()
        except:
            return
            
        # Delete voxel VAO and VBOs if they exist
        if self.vao != 0:
            glDeleteVertexArrays(1, [self.vao])
            self.vao = 0
        if self.vbo_vertices != 0:
            glDeleteBuffers(1, [self.vbo_vertices])
            self.vbo_vertices = 0
        if self.vbo_colors != 0:
            glDeleteBuffers(1, [self.vbo_colors])
            self.vbo_colors = 0
            
        # Delete grid VAO and VBOs if they exist
        if self.grid_vao != 0:
            glDeleteVertexArrays(1, [self.grid_vao])
            self.grid_vao = 0
        if self.grid_vbo_vertices != 0:
            glDeleteBuffers(1, [self.grid_vbo_vertices])
            self.grid_vbo_vertices = 0

    def initializeGL(self):
        """Initialize OpenGL settings"""
        glClearColor(0.0, 0.0, 0.0, 1.0)  # Black background
        glEnable(GL_DEPTH_TEST)  # Enable depth testing for 3D
        glEnable(GL_BLEND)
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
        glEnable(GL_CULL_FACE)
        glCullFace(GL_BACK)
        glShadeModel(GL_SMOOTH)  # Smooth shading
        
        # Initialize VBO/VAO for grid rendering (static - only needs to be done once)
        self._setup_grid_buffers()

    def resizeGL(self, width, height):
        """Handle window resize"""
        # Set flag indicating we're currently resizing
        self.resizing = True
        
        glViewport(0, 0, width, height)
        glMatrixMode(GL_PROJECTION)
        glLoadIdentity()
        gluPerspective(45, width / height, 0.1, 100.0)
        glMatrixMode(GL_MODELVIEW)
        
        # Start timer to reset the resizing flag after a pause in resize events
        self.resize_timer.stop()
        self.resize_timer.start(100)  # Wait 100ms after last resize event

    def _end_resize_operation(self):
        """Called when resize operation ends"""
        self.resizing = False
        # Trigger final update after resize completes
        self.update()

    def update_camera(self):
        """Update camera based on selected POV mode"""
        selector_x = self.selector_pos[2] * self.cell_size
        selector_y = (self.grid_size - 1 - self.selector_pos[1]) * self.cell_size
        selector_z = self.selector_pos[0] * self.cell_size
        
        if self.pov_mode == 0:  # Third person mode
            # New third person logic: behind, above, looking down
            center_x = selector_x + self.cell_size / 2
            center_y = selector_y + self.cell_size / 2
            center_z = selector_z + self.cell_size / 2

            distance = -9 #-7
            height = -2 #-4 #4
            pitch = -30 #-90
            
            # Reverse order of operations for OpenGL
            glTranslatef(0, 0, distance) # Step 5: move camera away from selector
            glRotatef(pitch, 1, 0, 0)  # Step 4: Pitch down to look at selector
            glRotatef(math.degrees(self.camera_direction), 0, 0, 1) # Step 3: Rotate around Z-axis
            glTranslatef(0, height, 0) # Step 2: Elevate camera
            glTranslatef(-center_x, -center_y, -center_z) # Step 1: Center on selector
        elif self.pov_mode == 1:  # First person mode
            # Camera positioned at selector position with appropriate eye-level height
            # For first person, position camera at selector with an eye-level height to see ground below
            eye_level = 2.0  # Higher eye level to see the ground beneath and have proper perspective
            
            # Set camera to be at the selector's position but slightly above
            # Apply rotation around Y-axis for left/right turning, but start facing +Y axis
            # To make +Y the default forward direction, we need to rotate from the default -Z facing
            # By -90 degrees around X-axis first, then apply the Y-axis rotation for direction
            glRotatef(-90, 1, 0, 0)  # Rotate to make Y-axis forward (from default -Z)
            glRotatef(math.degrees(self.camera_direction), 0, 0, 1)    # Then rotate around Z-axis for left/right
            # Offset camera to center it in the grid cell and at proper eye level
            # Move camera to center of the grid cube for proper viewing
            # Move camera forward by 1 grid unit in the Y direction (accounting for inverted Y rendering)
            glTranslatef(-selector_x - self.cell_size/2, -(selector_y + eye_level - 2*self.cell_size), -selector_z - self.cell_size/2)
        else:  # Free camera mode (pov_mode == 2)
            # Use the default camera controls
            glRotatef(self.camera_angle_x, 1, 0, 0)
            glRotatef(self.camera_angle_y, 0, 1, 0)
            glTranslatef(-self.camera_x, -self.camera_y, -self.camera_z)

    def paintGL(self):
        """Render the 3D scene"""
        # If we're currently resizing, skip all rendering for maximum responsiveness
        if self.resizing:
            return  # Don't render anything during resize operations
            
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
        glMatrixMode(GL_MODELVIEW)
        glLoadIdentity()
        
        # If loading is not complete, just show a simpler scene
        if not self.loading_complete:
            # Draw a simple basic scene with just the grid
            glClearColor(0.0, 0.0, 0.0, 1.0)  # Black background
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
            
            # Draw just the basic grid using VBO/VAO (even in loading state)
            self.draw_grid_vbo()
            
            # Still update minimap if callback available (though entities will be empty)
            if self.minimap_callback:
                self.minimap_callback({}, self.selector_pos, self.pov_mode, self.camera_direction)
            return
        
        # Normal rendering when loading is complete
        # Update camera based on POV mode
        self.update_camera()
        
        # Draw the grid using VBO/VAO for better performance
        self.draw_grid_vbo()
        
        # Draw entities (8x8x1 foundations) - restore original method for color fidelity
        self.draw_entities()
        
        # Draw selector (on top of everything else)
        self.draw_selector()
        
        # Draw XYZ axis indicator
        self.draw_axis_indicator()
        
        # Draw UI elements - this replaces the problematic paintEvent drawing
        # Skip UI elements during resize for better performance
        if not self.resizing:
            self._draw_ui_elements()
        
        # Update minimap via callback if provided
        if self.minimap_callback:
            # Get entity data in the format expected by the minimap
            entities_for_minimap = {}
            if self.data_loaded:  # Only populate if data is actually loaded
                for key, value in self.entities.items():
                    entities_for_minimap[key] = {
                        'x': value['x'],
                        'y': value['y'],  # Added y coordinate
                        'z': value['z'],
                        'color': value['color']
                    }
            self.minimap_callback(entities_for_minimap, self.selector_pos, self.pov_mode, self.camera_direction)
        
        # Update minimap position if it exists
        if hasattr(self, 'minimap_widget') and self.minimap_widget:
            # Position the minimap in the top-right corner of the 3D view
            x_pos = self.width() - self.minimap_widget.width() - 10
            y_pos = 10
            self.minimap_widget.setGeometry(x_pos, y_pos, self.minimap_widget.width(), self.minimap_widget.height())
    
    def draw_grid_vbo(self):
        """Draw the 3D grid using VBO/VAO for better performance"""
        if self.grid_vao != 0 and self.grid_vbo_vertices != 0 and self.grid_line_count > 0:
            # Set grid line properties
            glColor3f(0.5, 0.5, 0.5)  # Gray grid lines
            glLineWidth(1.0)
            
            # Use the VAO and draw the lines
            glBindVertexArray(self.grid_vao)
            glDrawArrays(GL_LINES, 0, self.grid_line_count)  # Draw using line primitives
            glBindVertexArray(0)

    def draw_grid_basic(self):
        """Draw a simplified grid for loading state"""
        glColor3f(0.3, 0.3, 0.3)  # Lighter grey for loading
        glLineWidth(1.0)
        
        # Draw grid lines in X-Y plane at each Z level
        for z in range(self.grid_size):
            for i in range(self.grid_size + 1):
                # Lines along X axis
                glBegin(GL_LINES)
                glVertex3f(0, i * self.cell_size, z * self.cell_size)
                glVertex3f(self.grid_size * self.cell_size, i * self.cell_size, z * self.cell_size)
                glEnd()
                
                # Lines along Y axis
                glBegin(GL_LINES)
                glVertex3f(i * self.cell_size, 0, z * self.cell_size)
                glVertex3f(i * self.cell_size, self.grid_size * self.cell_size, z * self.cell_size)
                glEnd()



    def _draw_ui_elements(self):
        """Draw all UI elements in the proper OpenGL context"""
        # Filter out elements that are not visible
        visible_elements = [elem for elem in self.ui_elements if elem.visible]
        
        # Group elements by type and sort by z_order within each group
        # First, draw all OpenGL-based elements (rectangles) in z-order
        opengl_elements = [elem for elem in visible_elements 
                          if isinstance(elem, (TranslucentRect))]
        opengl_elements.sort(key=lambda elem: elem.z_order)
        
        for element in opengl_elements:
            element.draw(self)
        
        # Then, draw all text elements using QPainter overlay in z-order
        text_elements = [elem for elem in visible_elements if isinstance(elem, TextElement)]
        text_elements.sort(key=lambda elem: elem.z_order)
        
        if text_elements:
            # Only create painter once for all text elements
            painter = QPainter(self)
            painter.setRenderHint(QPainter.Antialiasing, False)
            painter.setRenderHint(QPainter.TextAntialiasing, True)
            
            for element in text_elements:
                painter.setFont(element.font)
                painter.setPen(element.color)
                painter.drawText(element.x, element.y, element.text)
            
            painter.end()

    def draw_translucent_rect_opengl(self, x, y, width, height, opacity, color=(0.0, 0.0, 0.0)):
        """Draw a translucent rectangle using OpenGL commands to avoid depth buffer conflicts"""
        # Get current viewport size
        viewport = glGetIntegerv(GL_VIEWPORT)
        win_width, win_height = viewport[2], viewport[3]
        
        # Save current states
        glPushAttrib(GL_ALL_ATTRIB_BITS)
        
        # Switch to orthographic projection for 2D overlay
        glMatrixMode(GL_PROJECTION)
        glPushMatrix()
        glLoadIdentity()
        glOrtho(0, win_width, win_height, 0, -1, 1)  # Screen coordinates (0,0 at top-left)
        
        glMatrixMode(GL_MODELVIEW)
        glPushMatrix()
        glLoadIdentity()
        
        # Configure for 2D overlay without depth testing (ensure it's on top)
        glDisable(GL_DEPTH_TEST)
        glDepthMask(GL_FALSE)  # Don't write to depth buffer
        glEnable(GL_BLEND)
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
        
        # Draw the filled rectangle
        glColor4f(color[0], color[1], color[2], opacity)  # Color with specified opacity
        glBegin(GL_QUADS)
        glVertex2f(x, y)
        glVertex2f(x + width, y)
        glVertex2f(x + width, y + height)
        glVertex2f(x, y + height)
        glEnd()
        
        # Restore states
        glPopAttrib()
        glMatrixMode(GL_PROJECTION)
        glPopMatrix()
        glMatrixMode(GL_MODELVIEW)
        glPopMatrix()

    def draw_rect_border_opengl(self, x, y, width, height, color):
        """Draw a border around a rectangle using OpenGL commands"""
        viewport = glGetIntegerv(GL_VIEWPORT)
        win_width, win_height = viewport[2], viewport[3]
        
        # Save current states
        glPushAttrib(GL_ALL_ATTRIB_BITS)
        
        # Switch to orthographic projection for 2D overlay
        glMatrixMode(GL_PROJECTION)
        glPushMatrix()
        glLoadIdentity()
        glOrtho(0, win_width, win_height, 0, -1, 1)  # Screen coordinates (0,0 at top-left)
        
        glMatrixMode(GL_MODELVIEW)
        glPushMatrix()
        glLoadIdentity()
        
        # Configure for 2D overlay without depth testing
        glDisable(GL_DEPTH_TEST)
        glDepthMask(GL_FALSE)  # Don't write to depth buffer
        glEnable(GL_BLEND)
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
        
        # Draw the border lines
        glColor3f(color[0], color[1], color[2])  # Border color
        glLineWidth(2.0)  # Border thickness
        glBegin(GL_LINE_LOOP)  # Use LINE_LOOP to draw a complete rectangle border
        glVertex2f(x, y)           # Top-left
        glVertex2f(x + width, y)   # Top-right
        glVertex2f(x + width, y + height)  # Bottom-right
        glVertex2f(x, y + height)  # Bottom-left
        glEnd()
        glLineWidth(1.0)
        
        # Restore states
        glPopAttrib()
        glMatrixMode(GL_PROJECTION)
        glPopMatrix()
        glMatrixMode(GL_MODELVIEW)
        glPopMatrix()

    def _update_ui_elements(self):
        """Update UI elements with current state"""
        # Update selector position text
        pos_text_elem = next((elem for elem in self.ui_elements if isinstance(elem, TextElement) and "Selector X,Y,Z:" in getattr(elem, 'text', '')), None)
        if pos_text_elem:
            pos_text_elem.text = f"Selector X,Y,Z: {self.selector_pos[2]},{self.selector_pos[1]},{self.selector_pos[0]}"
        
        # Update POV mode text
        pov_text_elem = next((elem for elem in self.ui_elements if isinstance(elem, TextElement) and elem.text.startswith("POV Mode:")), None)
        if pov_text_elem:
            pov_text_elem.text = f"POV Mode: {self.pov_modes[self.pov_mode]}"
        
        # Trigger update to redraw UI
        self.update()

    def draw_grid(self):
        """Draw the 3D grid"""
        glColor3f(0.5, 0.5, 0.5)  # Gray grid lines
        glLineWidth(1.0)
        
        # Draw grid lines in X-Y plane at each Z level
        for z in range(self.grid_size):
            for i in range(self.grid_size + 1):
                # Lines along X axis
                glBegin(GL_LINES)
                glVertex3f(0, i * self.cell_size, z * self.cell_size)
                glVertex3f(self.grid_size * self.cell_size, i * self.cell_size, z * self.cell_size)
                glEnd()
                
                # Lines along Y axis
                glBegin(GL_LINES)
                glVertex3f(i * self.cell_size, 0, z * self.cell_size)
                glVertex3f(i * self.cell_size, self.grid_size * self.cell_size, z * self.cell_size)
                glEnd()

    def draw_entities(self):
        """Draw the 8x8x1 entity foundations in 3D"""
        # Only draw entities if data has been loaded
        if self.data_loaded:
            # Draw all entities currently in the main dictionary (unselected entities) from all Z levels
            for pos_key, entity in self.entities.items():
                self.draw_single_entity(entity['x'] * self.cell_size, 
                                        (self.grid_size - 1 - entity['y']) * self.cell_size, 
                                        entity['z'] * self.cell_size, 
                                        entity['color'],
                                        entity)  # Pass the entity data

        # If an entity is selected, draw it at the selector's current position
        if self.is_selected and self.selected_entity_data:
            x = self.selector_pos[2] * self.cell_size  # Using index 2 for X (pos[0]=z, pos[1]=y, pos[2]=x)
            y = (self.grid_size - 1 - self.selector_pos[1]) * self.cell_size
            z = self.selector_pos[0] * self.cell_size
            self.draw_single_entity(x, y, z, self.selected_entity_data['color'], self.selected_entity_data)

    def draw_single_entity(self, x, y, z, color, entity_data=None):
        """Draw a single entity foundation and its 3D model"""
        # Draw the thin foundation at the bottom of the grid cube
        # Set color based on entity type with higher transparency for 3D model placement
        if color == 'blue':
            glColor4f(0.0, 0.5, 1.0, 0.3)  # Blue with high transparency
        elif color == 'red':
            glColor4f(1.0, 0.3, 0.3, 0.3)  # Red with high transparency
        else:
            glColor4f(0.5, 0.5, 0.5, 0.3)  # Gray with high transparency

        # Draw the thin foundation cube at the bottom of the grid cube
        foundation_height = self.cell_size * 0.1  # Thin foundation
        self.draw_flat_cube(x, y, z, self.cell_size, foundation_height, is_entity=True)
        
        # Draw the 3D voxel model that fits within the same space
        if entity_data and 'voxel_model' in entity_data and entity_data['voxel_model']:
            self.draw_3d_model(x, y, z, entity_data['voxel_model'], self.cell_size)

    def draw_flat_cube(self, x, y, z, size, height, is_entity=False):
        """Draw a thin flat cube (foundation) at the specified position"""
        if is_entity:
            glBegin(GL_QUADS)
            # Top face (visible top of foundation)
            glVertex3f(x, y, z + height)
            glVertex3f(x + size, y, z + height)
            glVertex3f(x + size, y + size, z + height)
            glVertex3f(x, y + size, z + height)
            # Bottom face (bottom of foundation)
            glVertex3f(x, y, z)
            glVertex3f(x, y + size, z)
            glVertex3f(x + size, y + size, z)
            glVertex3f(x + size, y, z)
            # Front face (facing -Y direction)
            glVertex3f(x, y, z)
            glVertex3f(x, y, z + height)
            glVertex3f(x + size, y, z + height)
            glVertex3f(x + size, y, z)
            # Back face (facing +Y direction)
            glVertex3f(x, y + size, z)
            glVertex3f(x + size, y + size, z)
            glVertex3f(x + size, y + size, z + height)
            glVertex3f(x, y + size, z + height)
            # Right face (facing +X direction)
            glVertex3f(x + size, y, z)
            glVertex3f(x + size, y + size, z)
            glVertex3f(x + size, y + size, z + height)
            glVertex3f(x + size, y, z + height)
            # Left face (facing -X direction)
            glVertex3f(x, y, z)
            glVertex3f(x, y, z + height)
            glVertex3f(x, y + size, z + height)
            glVertex3f(x, y + size, z)
            glEnd()
        else:
            # Draw wireframe cube for selector - implement if needed
            pass

    def draw_3d_model(self, base_x, base_y, base_z, voxel_model, cell_size):
        """Draw a 3D voxel model that fills the entire grid cube space independently"""
        # The 8x8x8 model should scale to completely fill the grid cube space
        # Each voxel in the 8x8x8 model maps to 1/8th of the grid cube in each dimension
        voxel_size = cell_size / 8.0  # Each voxel takes 1/8 of the grid cube size
        
        for voxel in voxel_model:
            if len(voxel) >= 6:  # Ensure we have x,y,z,r,g,b
                orig_x, orig_y, orig_z, r, g, b = voxel[:6]
                
                # Apply rotation transformation for entity models:
                # Rotate 90 degrees around X-axis so top faces +Y and bottom faces -Y
                # New X = orig_x (unchanged)
                # New Y = orig_z (original Z becomes new Y)
                # New Z = -orig_y (original Y becomes negative Z)
                if self.rotate_entities:
                    rot_x = orig_x
                    rot_y = orig_z
                    rot_z = -orig_y
                    rot_z = 7 + rot_z  # Adjust range to [0,7] again
                else:
                    # Use original coordinates without rotation
                    rot_x = orig_x
                    rot_y = orig_y
                    rot_z = orig_z
                
                # Calculate world position for this voxel using rotated coordinates
                # rot_x,rot_y,rot_z are in 0-7 space, convert to world coordinates within the entity's grid cube
                world_x = base_x + (rot_x / 8.0) * cell_size
                # Apply proper Y inversion for correct rendering (since Y is inverted in rendering)
                world_y = base_y + (7 - rot_y) / 8.0 * cell_size
                world_z = base_z + (rot_z / 8.0) * cell_size
                
                # Draw a small cube for this voxel
                self.draw_voxel_at(world_x, world_y, world_z, voxel_size, r, g, b)

    def draw_voxel_at(self, x, y, z, size, r, g, b):
        """Draw a single voxel cube at the specified position with given color"""
        # Save current matrix
        glPushMatrix()
        
        # Translate to the position of the voxel
        glTranslatef(x, y, z)
        
        # Set the color for this voxel
        glColor3f(r, g, b)
        
        # Define vertices of a cube with the given size
        half_size = size / 2.0
        vertices = [
            -half_size, -half_size, -half_size,
            half_size, -half_size, -half_size,
            half_size,  half_size, -half_size,
            -half_size,  half_size, -half_size,
            -half_size, -half_size,  half_size,
            half_size, -half_size,  half_size,
            half_size,  half_size,  half_size,
            -half_size,  half_size,  half_size
        ]

        # Define faces of the cube
        faces = [
            [0, 1, 2, 3],  # Back face
            [4, 5, 6, 7],  # Front face
            [0, 4, 7, 3],  # Left face 
            [1, 2, 6, 5],  # Right face
            [3, 2, 6, 7],  # Top face
            [0, 1, 5, 4]   # Bottom face
        ]

        # Draw each face
        glBegin(GL_QUADS)
        for face in faces:
            for vertex_index in face:
                glVertex3f(vertices[vertex_index * 3],
                          vertices[vertex_index * 3 + 1], 
                          vertices[vertex_index * 3 + 2])
        glEnd()
        
        # Restore matrix
        glPopMatrix()

    def draw_selector(self):
        """Draw the selection indicator in 3D"""
        x = self.selector_pos[2] * self.cell_size  # Using index 2 for X
        y = (self.grid_size - 1 - self.selector_pos[1]) * self.cell_size  # Using index 1 for Y
        z = self.selector_pos[0] * self.cell_size  # Using index 0 for Z
        
        # Check if there's an entity at the exact selector position (same z level, same x, same y)
        target_pos_key = f"{self.selector_pos[0]},{self.selector_pos[2]}"  # Using z (index 0) and x (index 2)
        has_entity = False
        
        # Check if there's an entity at the target position with matching Y
        if target_pos_key in self.entities:
            entity = self.entities[target_pos_key]
            # The entity should be at the same Y position as the selector's Y position
            if entity['y'] == self.selector_pos[1]:  # Compare entity's y with selector's y (index 1)
                has_entity = True
        
        if not self.is_selected and has_entity:
            # Purple when hovering over a piece (not selected yet but entity exists at exact position)
            glColor3f(0.8, 0.0, 0.8)  # Purple (0.8, 0, 0.8)
        elif self.is_selected:
            # Check if target is occupied by another entity (not the one being moved)
            target_occupied = False
            if target_pos_key in self.entities:
                # Check if the entity is at the same Y position as where we're trying to move
                entity = self.entities[target_pos_key]
                if entity['y'] == self.selector_pos[1]:  # Compare entity's y with target y
                    target_occupied = True
            
            if target_occupied:
                glColor3f(1.0, 1.0, 0.0) # Yellow for invalid move target
            else:
                glColor3f(0.0, 1.0, 0.0) # Green for valid move target
        else:
            glColor3f(1.0, 1.0, 1.0) # Default white
        
        # Draw selection cube with slightly larger size to make it visible
        selector_size = self.cell_size * 1.1
        self.draw_wireframe_cube(x - (selector_size - self.cell_size)/2, 
                                y - (selector_size - self.cell_size)/2, 
                                z - (selector_size - self.cell_size)/2, 
                                selector_size)

    def draw_wireframe_cube(self, x, y, z, size):
        """Draw a wireframe cube for the selector"""
        # Save current matrix
        glPushMatrix()
        glTranslatef(x, y, z)
        
        # Define vertices of a cube with the given size
        half_size = size / 2.0
        vertices = [
            -half_size, -half_size, -half_size,
            half_size, -half_size, -half_size,
            half_size,  half_size, -half_size,
            -half_size,  half_size, -half_size,
            -half_size, -half_size,  half_size,
            half_size, -half_size,  half_size,
            half_size,  half_size,  half_size,
            -half_size,  half_size,  half_size
        ]

        # Define edges of the cube
        edges = [
            (0, 1), (1, 2), (2, 3), (3, 0),  # Back face
            (4, 5), (5, 6), (6, 7), (7, 4),  # Front face
            (0, 4), (1, 5), (2, 6), (3, 7)   # Connecting edges
        ]

        # Draw the wireframe
        glLineWidth(2.0)  # Thicker lines for selector
        glBegin(GL_LINES)
        for start_idx, end_idx in edges:
            glVertex3f(vertices[start_idx * 3],
                      vertices[start_idx * 3 + 1],
                      vertices[start_idx * 3 + 2])
            glVertex3f(vertices[end_idx * 3],
                      vertices[end_idx * 3 + 1],
                      vertices[end_idx * 3 + 2])
        glEnd()
        glLineWidth(1.0)
        
        # Restore matrix
        glPopMatrix()

    def draw_axis_indicator(self):
        """Draw a small XYZ axis indicator in the corner"""
        # Save current matrix
        glPushMatrix()
        
        # Position the axis indicator in the lower-left corner
        # Move to the front-bottom-left of the viewable area
        glTranslatef(0.5, self.grid_size * self.cell_size - 0.5, 0.5)
        
        # Draw the axes
        glLineWidth(3.0)
        
        # X axis (Red)
        glColor3f(1.0, 0.0, 0.0)
        glBegin(GL_LINES)
        glVertex3f(0, 0, 0)
        glVertex3f(0.5, 0, 0)
        glEnd()
        
        # Y axis (Green)
        glColor3f(0.0, 1.0, 0.0)
        glBegin(GL_LINES)
        glVertex3f(0, 0, 0)
        glVertex3f(0, 0.5, 0)
        glEnd()
        
        # Z axis (Blue)
        glColor3f(0.0, 0.0, 1.0)
        glBegin(GL_LINES)
        glVertex3f(0, 0, 0)
        glVertex3f(0, 0, 0.5)
        glEnd()
        
        glLineWidth(1.0)
        
        # Restore matrix
        glPopMatrix()

    def keyPressEvent(self, event):
        """Handle keyboard input for camera movement, selector movement, and POV mode"""
        key = event.key()
        
        # Check for POV mode toggle first ('0' key)
        if key == ord('0') or key == Qt.Key_0:
            self.pov_mode = (self.pov_mode + 1) % 3  # Cycle through 0, 1, 2
            print(f"POV Mode: {self.pov_modes[self.pov_mode]}")
            self._update_ui_elements()
            self.update()
            return
        
        # Check for entity rotation toggle ('F' key)
        if key == ord('F') or key == Qt.Key_F:
            self.rotate_entities = not self.rotate_entities
            print(f"Entity rotation: {'ON' if self.rotate_entities else 'OFF'}")
            # Regenerate the voxel buffers since rotation affects voxel positions
            if self.data_loaded:  # Only regenerate if data is loaded
                self._setup_voxel_buffers()
            self.update()
            return
        
        # Handle camera movement keys based on POV mode
        camera_moved = False
        
        # Handle Z and X keys first for consistent Z-level movement across all modes
        if key == ord('Z') or key == ord('z'):
            # Move selector down in Z-axis (decrease Z)
            if self.selector_pos[0] > 0:
                self.selector_pos[0] -= 1
                camera_moved = True
        elif key == ord('X') or key == ord('x'):
            # Move selector up in Z-axis (increase Z)
            if self.selector_pos[0] < self.grid_size - 1:
                self.selector_pos[0] += 1
                camera_moved = True
        elif key == ord('W') or key == ord('w'):
            if self.pov_mode == 2:  # Free camera mode - move camera
                self.camera_x -= math.sin(math.radians(self.camera_angle_y)) * self.move_speed
                self.camera_y += math.cos(math.radians(self.camera_angle_y)) * self.move_speed
                camera_moved = True
            elif self.pov_mode in [0, 1]:  # First or Third person mode - rotate to face forward (+Y)
                self.camera_direction = 0  # Face forward (+Y direction)
                camera_moved = True
        elif key == ord('S') or key == ord('s'):
            if self.pov_mode == 2:  # Free camera mode - move camera
                self.camera_x += math.sin(math.radians(self.camera_angle_y)) * self.move_speed
                self.camera_y -= math.cos(math.radians(self.camera_angle_y)) * self.move_speed
                camera_moved = True
            elif self.pov_mode in [0, 1]:  # First or Third person mode - rotate to face back (-Y)
                self.camera_direction = math.pi  # Face back (-Y direction)
                camera_moved = True
        elif key == ord('A') or key == ord('a'):
            if self.pov_mode == 2:  # Free camera mode - move camera
                self.camera_x -= math.cos(math.radians(self.camera_angle_y)) * self.move_speed
                self.camera_y -= math.sin(math.radians(self.camera_angle_y)) * self.move_speed
                camera_moved = True
            elif self.pov_mode in [0, 1]:  # First or Third person mode - rotate to face left (-X)
                self.camera_direction = -math.pi/2  # Face left (-X direction)
                camera_moved = True
        elif key == ord('D') or key == ord('d'):
            if self.pov_mode == 2:  # Free camera mode - move camera
                self.camera_x += math.cos(math.radians(self.camera_angle_y)) * self.move_speed
                self.camera_y += math.sin(math.radians(self.camera_angle_y)) * self.move_speed
                camera_moved = True
            elif self.pov_mode in [0, 1]:  # First or Third person mode - rotate to face right (+X)
                self.camera_direction = math.pi/2  # Face right (+X direction)
                camera_moved = True
        elif key == ord('Q') or key == ord('q'):  # Q key to turn left
            if self.pov_mode in [0, 1]:  # For first and third person mode
                self.camera_direction -= math.pi / 8  # Turn 22.5 degrees left
                camera_moved = True
        elif key == ord('E') or key == ord('e'):  # E key to turn right
            if self.pov_mode in [0, 1]:  # For first and third person mode
                self.camera_direction += math.pi / 8  # Turn 22.5 degrees right
                camera_moved = True
        elif key == Qt.Key_Return or key == Qt.Key_Enter:  # Enter key to select/deselect or confirm move
            target_pos_key = f"{self.selector_pos[0]},{self.selector_pos[2]}"  # Using z and x for position key
            
            if not self.is_selected:
                # Attempt to select an entity at the X,Z coordinates (regardless of Y level)
                if self.data_loaded and target_pos_key in self.entities:
                    # Check if there's an entity at the same Y level
                    entity_at_pos = None
                    for key, entity in self.entities.items():
                        if key == target_pos_key and entity['y'] == self.selector_pos[1]:
                            entity_at_pos = key
                            break
                    if entity_at_pos:
                        self.is_selected = True
                        self.selected_entity_key_when_picked_up = entity_at_pos
                        self.selected_entity_data = self.entities.pop(entity_at_pos)  # Remove from entities
                        # Regenerate the voxel buffers since entity was removed
                        self._setup_voxel_buffers()
            else:
                # Confirm move or deselect
                target_pos_key = f"{self.selector_pos[0]},{self.selector_pos[2]}"  # Using z and x for position key
                target_occupied = False
                if target_pos_key in self.entities:  # Target occupied by another entity
                    # Check if the entity is at the same Y position as where we're trying to move
                    if self.entities[target_pos_key]['y'] == self.selector_pos[1]:
                        target_occupied = True
                
                if target_occupied:  # Target occupied by another entity
                    # Invalidate move, revert entity to its original position
                    self.entities[self.selected_entity_key_when_picked_up] = self.selected_entity_data
                    # Regenerate the voxel buffers since entity was returned
                    self._setup_voxel_buffers()
                else:  # Target is empty
                    # Commit the move
                    self.selected_entity_data['z'] = self.selector_pos[0]
                    self.selected_entity_data['x'] = self.selector_pos[2]
                    self.selected_entity_data['y'] = self.selector_pos[1]
                    self.entities[target_pos_key] = self.selected_entity_data
                    # Regenerate the voxel buffers since entity was moved
                    self._setup_voxel_buffers()
                
                # Reset selection state
                self.is_selected = False
                self.selected_entity_key_when_picked_up = None
                self.selected_entity_data = None
            
            # Force update to refresh display immediately after selection/deselection
            self.update()
        else:
            # Movement keys (different behavior depending on POV mode)
            old_selector_pos = self.selector_pos[:]
            new_pos = old_selector_pos[:]
            
            # For first and third-person modes, movement should be relative to camera direction
            if self.pov_mode in [0, 1]:  # First or Third person mode
                if key == Qt.Key_Up:  # Move forward in facing direction
                    # Calculate movement based on camera direction
                    delta_x = math.sin(self.camera_direction)
                    delta_y = math.cos(self.camera_direction)
                    
                    # Since Y is inverted in rendering (lower grid Y appears higher), 
                    # we need to account for this in grid coordinate changes
                    new_grid_x = self.selector_pos[2] + round(delta_x)
                    new_grid_y = self.selector_pos[1] - round(delta_y)  # Negative because of inverted Y axis
                    
                    # Apply boundaries
                    if 0 <= new_grid_x < self.grid_size:
                        new_pos[2] = int(new_grid_x)
                    if 0 <= new_grid_y < self.grid_size:
                        new_pos[1] = int(new_grid_y)
                        
                elif key == Qt.Key_Down:  # Move backward
                    delta_x = math.sin(self.camera_direction)
                    delta_y = math.cos(self.camera_direction)
                    
                    new_grid_x = self.selector_pos[2] - round(delta_x)
                    new_grid_y = self.selector_pos[1] + round(delta_y)  # Positive Y in opposite direction
                    
                    # Apply boundaries
                    if 0 <= new_grid_x < self.grid_size:
                        new_pos[2] = int(new_grid_x)
                    if 0 <= new_grid_y < self.grid_size:
                        new_pos[1] = int(new_grid_y)
                        
                elif key == Qt.Key_Left:  # Strafe left
                    # Move perpendicular 90 degrees CCW from facing direction
                    delta_x = math.sin(self.camera_direction - math.pi/2) 
                    delta_y = math.cos(self.camera_direction - math.pi/2)
                    
                    new_grid_x = self.selector_pos[2] + round(delta_x)
                    new_grid_y = self.selector_pos[1] - round(delta_y)
                    
                    # Apply boundaries
                    if 0 <= new_grid_x < self.grid_size:
                        new_pos[2] = int(new_grid_x)
                    if 0 <= new_grid_y < self.grid_size:
                        new_pos[1] = int(new_grid_y)
                        
                elif key == Qt.Key_Right:  # Strafe right
                    # Move perpendicular 90 degrees CW from facing direction  
                    delta_x = math.sin(self.camera_direction + math.pi/2) 
                    delta_y = math.cos(self.camera_direction + math.pi/2)
                    
                    new_grid_x = self.selector_pos[2] + round(delta_x)
                    new_grid_y = self.selector_pos[1] - round(delta_y)
                    
                    # Apply boundaries
                    if 0 <= new_grid_x < self.grid_size:
                        new_pos[2] = int(new_grid_x)
                    if 0 <= new_grid_y < self.grid_size:
                        new_pos[1] = int(new_grid_y)
            else:
                # For free camera mode, use original movement
                if key == Qt.Key_Up and self.selector_pos[1] > 0:
                    new_pos[1] -= 1  # Moving up decreases Y to compensate for inverted drawing
                elif key == Qt.Key_Down and self.selector_pos[1] < self.grid_size - 1:
                    new_pos[1] += 1  # Moving down increases Y to compensate for inverted drawing
                elif key == Qt.Key_Left and self.selector_pos[2] > 0:
                    new_pos[2] -= 1  # Moving left decreases X
                elif key == Qt.Key_Right and self.selector_pos[2] < self.grid_size - 1:
                    new_pos[2] += 1  # Moving right increases X
            
            # Only update selector position if it actually changed
            if new_pos != old_selector_pos:
                self.selector_pos = new_pos
                camera_moved = True  # Update the display if selector moved
        
        # Update the display after any action
        if camera_moved:
            self.update()
        
        # Also update the minimap for all changes that affect position, camera direction, or pov mode
        # (movement keys, selection, pov changes, etc.)
        if (camera_moved or 
            key in [Qt.Key_Up, Qt.Key_Down, Qt.Key_Left, Qt.Key_Right, Qt.Key_Z, Qt.Key_X] or
            key == Qt.Key_Return or key == Qt.Key_Enter or 
            key == ord('0') or key == Qt.Key_0 or key == ord('Q') or key == ord('E')):  # pov mode change or rotation
            # Update UI elements to reflect changes in position or mode
            self._update_ui_elements()

    def resizeEvent(self, event):
        """Handle widget resize to position minimap properly"""
        super().resizeEvent(event)
        # Update minimap position if it exists
        if hasattr(self, 'minimap_widget') and self.minimap_widget:
            # Position the minimap in the top-right corner of the 3D view
            x_pos = self.width() - self.minimap_widget.width() - 10
            y_pos = 10
            self.minimap_widget.setGeometry(x_pos, y_pos, self.minimap_widget.width(), self.minimap_widget.height())