"""
2D PyBoard Application for PyOS
A 2D voxel viewer application adapted from voxel_viewer_2d_fixed+z_local.py
"""

from PySide6.QtWidgets import QVBoxLayout, QHBoxLayout, QWidget, QLabel, QPushButton, QSplitter
from PySide6.QtCore import Qt
from PySide6.QtGui import QPainter, QColor, QFont, QPen

import sys
import csv
import os

from apps.base_app import BaseApplication


class GridWidget(QWidget):
    def __init__(self, grid_size=12, cell_size=30):
        super().__init__()
        
        self.grid_size = grid_size
        self.cell_size = cell_size
        self.entities = {}
        self.selector_pos = [0, 0, 0]  # Starting position of selector [z, row, col] - will be updated in init_entities
        self.is_selected = False
        self.selected_entity_key_when_picked_up = None # The key of the cell the entity was removed from
        self.selected_entity_data = None # The entity data itself, removed from self.entities
        
        # Set size policy to expand
        self.setMinimumSize(grid_size * cell_size + 50, grid_size * cell_size + 60)
        # Allow expanding beyond the minimum size
        from PySide6.QtWidgets import QSizePolicy
        self.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        
        # Enable focus for keyboard events
        self.setFocusPolicy(Qt.StrongFocus)
        
        # Don't initialize entities here to avoid painting during construction
        # Will initialize when widget is shown
        self.initialized = False

    def showEvent(self, event):
        """Initialize entities when widget is first shown"""
        super().showEvent(event)
        if not self.initialized:
            self.init_entities()
            self.initialized = True

    def init_entities(self):
        """Initialize the entities on the grid"""
        # Clear existing entities
        self.entities = {}
        
        # Load data to determine entity positions
        self.load_data()
        
        # Initialize the selector to the Z-level of the first entity if present
        if self.entities:
            first_entity = next(iter(self.entities.values()))
            self.selector_pos = [first_entity['z'], first_entity['row'], first_entity['col']]
        else:
            self.selector_pos = [0, 0, 0]  # Default to Z=0 if no entities loaded
        
        # The widget will automatically paint when shown
        # No need to call update here as it will be called when the widget is painted
        
    def load_data(self):
        """Load entity data from CSV"""
        # Look for data in the shared pyboards_data directory
        script_dir = os.path.dirname(os.path.abspath(__file__))
        file_path = os.path.join(script_dir, 'pyboards_data', 'aggregated_data.csv')
        try:
            import pandas as pd
            df = pd.read_csv(file_path)
            attributes_df = df[(df['source_file'].str.contains('entity_attributes.csv', na=False)) & (df['entity_id'].isin(['adam', 'eve']))]
            for entity_id in ['adam', 'eve']:
                entity_attrs = attributes_df[(attributes_df['entity_id'] == entity_id) & (attributes_df['attribute'].isin(['x', 'z']))]
                if not entity_attrs.empty:
                    x_val = entity_attrs[entity_attrs['attribute'] == 'x']['value'].iloc[0]
                    z_val = entity_attrs[entity_attrs['attribute'] == 'z']['value'].iloc[0]
                    x = float(x_val) if isinstance(x_val, str) else x_val
                    z = float(z_val) if isinstance(z_val, str) else z_val
                    pos_key = f"{int(z)},{int(x)}"  # Using z as row, x as col
                    
                    # Load tilemap CSV for the entity
                    tilemap_csv = self.load_tilemap_csv(entity_id)
                    
                    self.entities[pos_key] = {'color': 'red' if entity_id == 'adam' else 'blue', 'z': int(z), 'row': int(z), 'col': int(x), 'id': entity_id, 'tilemap': tilemap_csv}
        except FileNotFoundError:
            print("aggregated_data.csv not found. Please generate it first.")
            # Add example entities if file not found with their tilemaps
            self.entities["1,2"] = {'color': 'red', 'z': 1, 'row': 1, 'col': 2, 'id': 'adam', 'tilemap': self.load_tilemap_csv('adam')}
            self.entities["3,4"] = {'color': 'blue', 'z': 3, 'row': 3, 'col': 4, 'id': 'eve', 'tilemap': self.load_tilemap_csv('eve')}
        except Exception as e:
            print(f"Error loading data: {e}")
            # Add example entities if error occurs with their tilemaps
            self.entities["1,2"] = {'color': 'red', 'z': 1, 'row': 1, 'col': 2, 'id': 'adam', 'tilemap': self.load_tilemap_csv('adam')}
            self.entities["3,4"] = {'color': 'blue', 'z': 3, 'row': 3, 'col': 4, 'id': 'eve', 'tilemap': self.load_tilemap_csv('eve')}

    def load_tilemap_csv(self, entity_id):
        """Load the 8x8 tilemap CSV file for an entity"""
        script_dir = os.path.dirname(os.path.abspath(__file__))
        entity_dir = os.path.join(script_dir, 'pyboards_data', 'entities', entity_id, 'tile_8x8')
        
        tilemap_data = []
        try:
            # Look for CSV files in the entity's tile_8x8 directory
            for file_name in os.listdir(entity_dir):
                if file_name.endswith('.csv'):
                    csv_path = os.path.join(entity_dir, file_name)
                    with open(csv_path, 'r') as f:
                        reader = csv.reader(f)
                        for row in reader:
                            tilemap_data.append(row)
                    break  # Use the first CSV file found
        except FileNotFoundError:
            print(f"Tilemap CSV not found for {entity_id}. Creating a default tilemap.")
            # Create a default 8x8 tilemap if CSV is not found
            for i in range(8):
                row = []
                for j in range(8):
                    if i == 0 or i == 7 or j == 0 or j == 7:
                        row.append("128,128,128")  # Border color
                    else:
                        row.append("255,255,255")  # Default white center
                tilemap_data.append(row)
        except Exception as e:
            print(f"Error loading tilemap for {entity_id}: {e}")
            # Create a default 8x8 tilemap if error occurs
            for i in range(8):
                row = []
                for j in range(8):
                    if i == 0 or i == 7 or j == 0 or j == 7:
                        row.append("128,128,128")  # Border color
                    else:
                        row.append("255,255,255")  # Default white center
                tilemap_data.append(row)
        
        return tilemap_data

    def paintEvent(self, event):
        """Paint the scene"""
        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing)
        
        # Draw the grid
        self.draw_grid(painter)
        
        # Draw entities
        self.draw_entities(painter)
        
        # Draw selector (on top of everything else)
        self.draw_selector(painter)

    def draw_grid(self, painter):
        """Draw the grid with labels"""
        # Draw the border around the grid area
        pen = QPen(QColor(128, 128, 128), 2)
        painter.setPen(pen)
        painter.drawRect(25, 15, self.grid_size * self.cell_size, self.grid_size * self.cell_size)
        
        # Draw inner grid lines
        # Vertical lines
        for i in range(self.grid_size + 1):
            painter.drawLine(25 + i * self.cell_size, 15, 
                             25 + i * self.cell_size, 15 + self.grid_size * self.cell_size)
        
        # Horizontal lines  
        for i in range(self.grid_size + 1):
            painter.drawLine(25, 15 + i * self.cell_size, 
                             25 + self.grid_size * self.cell_size, 15 + i * self.cell_size)
        
        # Draw column labels (0-11) - above the grid
        painter.setFont(QFont("Arial", 10))
        painter.setPen(QColor(255, 255, 255))  # White text
        for i in range(self.grid_size):
            col_label = f"X{i}"  # X0, X1, X2, ... X11
            # Position above the grid, centered horizontally within each cell
            painter.drawText(25 + i * self.cell_size + self.cell_size // 4, 12, col_label)
        
        # Draw row labels (0-11) - to the left of the grid
        for i in range(self.grid_size):
            row_label = f"Y{i}"  # Y0, Y1, Y2, ... Y11
            # Position to the left of the grid, vertically centered in each cell
            painter.drawText(5, 15 + i * self.cell_size + self.cell_size // 2 + 5, row_label)
        
        # Draw Z level indicator
        painter.setFont(QFont("Arial", 12, QFont.Bold))
        painter.setPen(QColor(255, 255, 0))  # Yellow text
        z_label = f"Z-Level: {self.selector_pos[0]}"
        # Position just below the grid area
        painter.drawText(25, 15 + self.grid_size * self.cell_size + 5, z_label)

    def draw_entities(self, painter):
        """Draw the entities on the grid"""
        # Draw all entities currently in the main dictionary (unselected entities) that are at the current Z level
        current_z_level = self.selector_pos[0]
        for pos_key, entity in self.entities.items():
            if entity['z'] == current_z_level:  # Only draw entities at the current Z level
                self.draw_single_entity(painter, entity['row'], entity['col'], entity['color'], entity.get('tilemap'))

        # If an entity is selected, draw it at the selector's current position
        if self.is_selected and self.selected_entity_data:
            self.draw_single_entity(painter, self.selector_pos[1], self.selector_pos[2], self.selected_entity_data['color'], self.selected_entity_data.get('tilemap'))

    def draw_single_entity(self, painter, row, col, color, tilemap=None):
        """Helper to draw a single entity with optional tilemap"""
        x = 25 + col * self.cell_size
        y = 15 + row * self.cell_size
        
        # Draw the background color first
        if color == 'blue':
            painter.setBrush(QColor(0, 128, 255))  # Blue
        elif color == 'red':
            painter.setBrush(QColor(255, 77, 77))  # Red
        else:
            painter.setBrush(QColor(128, 128, 128))  # Gray as default
        
        painter.drawRect(x + 2, y + 2, self.cell_size - 4, self.cell_size - 4)
        
        # Draw the tilemap if available
        if tilemap:
            self.draw_tilemap(painter, x + 2, y + 2, self.cell_size - 4, tilemap)

    def draw_tilemap(self, painter, x, y, size, tilemap):
        """Draw the 8x8 tilemap inside the entity, slightly smaller than the background"""
        # Calculate margin to make the tilemap smaller than the background
        margin = size * 0.15  # 15% margin around the tilemap for better visibility
        tilemap_size = size - 2 * margin  # Reduced size for the tilemap
        
        if tilemap_size <= 0:  # Ensure there's space for the tilemap
            return
        
        # Calculate the size for each tile in the 8x8 grid
        tile_size = tilemap_size / 8
        
        # Starting position with margin
        start_x = x + margin
        start_y = y + margin
        
        # Draw filled rectangles for each tile in the 8x8 grid
        for row_idx, row in enumerate(tilemap):
            for col_idx, rgb_str in enumerate(row):
                if row_idx < 8 and col_idx < 8:  # Ensure we stay within the 8x8 grid
                    # Calculate position for this tile
                    tile_x = start_x + col_idx * tile_size
                    tile_y = start_y + row_idx * tile_size
                    
                    # Parse RGB values from the string like "255,200,150"
                    try:
                        r, g, b = map(int, rgb_str.split(','))
                        color = QColor(r, g, b)
                    except ValueError:
                        # Default to white if parsing fails
                        color = QColor(255, 255, 255)
                    
                    # Draw filled rectangle for the tile
                    painter.fillRect(tile_x + 0.5, tile_y + 0.5, tile_size - 1, tile_size - 1, color)

    def draw_selector(self, painter):
        """Draw the selection indicator"""
        x = 25 + self.selector_pos[2] * self.cell_size  # Using column (index 2) for x position
        y = 15 + self.selector_pos[1] * self.cell_size  # Using row (index 1) for y position
        
        # Check if there's an entity at the exact selector position (same z level, same row, same column)
        # The entity dictionary key format is "{z},{col}", and entity['row'] gives the y position
        target_pos_key = f"{self.selector_pos[0]},{self.selector_pos[2]}"  # Using z (index 0) and column (index 2)
        has_entity = False
        
        # Check if there's an entity at the target position with matching Y (row)
        if target_pos_key in self.entities:
            entity = self.entities[target_pos_key]
            # The entity should be at the same Y position as the selector's Y position
            if entity['row'] == self.selector_pos[1]:  # Compare entity's row with selector's row (index 1)
                has_entity = True
        
        if not self.is_selected and has_entity:
            # Purple when hovering over a piece (not selected yet but entity exists at exact position)
            pen_color = QColor(204, 0, 204)  # Purple (204, 0, 204 is approximately 0.8, 0, 0.8 in 0-255 range)
        elif self.is_selected:
            # Check if target is occupied by another entity (not the one being moved)
            target_occupied = False
            if target_pos_key in self.entities:
                # Check if the entity is at the same Y position as where we're trying to move
                entity = self.entities[target_pos_key]
                if entity['row'] == self.selector_pos[1]:  # Compare entity's row with target row
                    target_occupied = True
            
            if target_occupied:
                pen_color = QColor(255, 255, 0) # Yellow for invalid move target
            else:
                pen_color = QColor(0, 255, 0) # Green for valid move target
        else:
            pen_color = QColor(255, 255, 255) # Default white
        
        painter.setPen(QPen(pen_color, 3))
        painter.setBrush(Qt.NoBrush)
        
        # Draw selection rectangle
        painter.drawRect(x, y, self.cell_size, self.cell_size)

    def keyPressEvent(self, event):
        """Handle keyboard input for selector movement and entity selection/movement"""
        key = event.key()
        
        if key == Qt.Key_Return or key == Qt.Key_Enter:  # Enter key to select/deselect or confirm move
            current_pos_key = f"{self.selector_pos[0]},{self.selector_pos[2]}"  # z,row,col -> using z and col for position key
            
            if not self.is_selected:
                # Attempt to select an entity
                if current_pos_key in self.entities:
                    self.is_selected = True
                    self.selected_entity_key_when_picked_up = current_pos_key
                    self.selected_entity_data = self.entities.pop(current_pos_key) # Remove from entities
            else:
                # Confirm move or deselect
                target_pos_key = f"{self.selector_pos[0]},{self.selector_pos[2]}"  # z,row,col -> using z and col for position key
                if target_pos_key in self.entities: # Target occupied by another entity
                    # Invalidate move, revert entity to its original position
                    self.entities[self.selected_entity_key_when_picked_up] = self.selected_entity_data
                else: # Target is empty
                    # Commit the move
                    self.selected_entity_data['z'] = self.selector_pos[0]
                    self.selected_entity_data['row'] = self.selector_pos[1]
                    self.selected_entity_data['col'] = self.selector_pos[2]
                    self.entities[target_pos_key] = self.selected_entity_data
                
                # Reset selection state
                self.is_selected = False
                self.selected_entity_key_when_picked_up = None
                self.selected_entity_data = None
        elif key == Qt.Key_Space:  # Alternative to Enter for select/deselect/confirm
            current_pos_key = f"{self.selector_pos[0]},{self.selector_pos[2]}"  # z,row,col -> using z and col for position key
            if not self.is_selected:
                if current_pos_key in self.entities:
                    self.is_selected = True
                    self.selected_entity_key_when_picked_up = current_pos_key
                    self.selected_entity_data = self.entities.pop(current_pos_key)
            else:
                target_pos_key = f"{self.selector_pos[0]},{self.selector_pos[2]}"  # z,row,col -> using z and col for position key
                if target_pos_key in self.entities:
                    self.entities[self.selected_entity_key_when_picked_up] = self.selected_entity_data
                else:
                    self.selected_entity_data['z'] = self.selector_pos[0]
                    self.selected_entity_data['row'] = self.selector_pos[1]
                    self.selected_entity_data['col'] = self.selector_pos[2]
                    self.entities[target_pos_key] = self.selected_entity_data
                
                # Reset selection state
                self.is_selected = False
                self.selected_entity_key_when_picked_up = None
                self.selected_entity_data = None
            self.update()
            return
        else:
            # Movement keys
            old_selector_pos = self.selector_pos[:]
            new_pos = old_selector_pos[:]
            
            if key == Qt.Key_Up and self.selector_pos[1] > 0:
                new_pos[1] -= 1  # Moving up decreases row
            elif key == Qt.Key_Down and self.selector_pos[1] < self.grid_size - 1:
                new_pos[1] += 1  # Moving down increases row
            elif key == Qt.Key_Left and self.selector_pos[2] > 0:
                new_pos[2] -= 1  # Moving left decreases column
            elif key == Qt.Key_Right and self.selector_pos[2] < self.grid_size - 1:
                new_pos[2] += 1  # Moving right increases column
            elif key == Qt.Key_Z and self.selector_pos[0] > 0:  # Z key to move down in Z-axis (decrease Z)
                new_pos[0] -= 1
            elif key == Qt.Key_X and self.selector_pos[0] < self.grid_size - 1:  # X key to move up in Z-axis (increase Z)
                new_pos[0] += 1
            
            # Only update selector position if it actually changed
            if new_pos != old_selector_pos:
                self.selector_pos = new_pos
        
        self.update() # Update the display after any action




class PyBoard2DApp(BaseApplication):
    def setup_ui(self):
        """Setup the 2D PyBoard application UI"""
        layout = QVBoxLayout(self)
        layout.setContentsMargins(10, 10, 10, 10)
        
        # Title
        title_label = QLabel("2D PyBoard Viewer")
        title_label.setStyleSheet("font-size: 18px; font-weight: bold; color: #ddd;")
        layout.addWidget(title_label)
        
        # Description
        desc_label = QLabel("A 2D voxel viewer with Z-level navigation")
        desc_label.setStyleSheet("color: #aaa; margin-top: 5px;")
        layout.addWidget(desc_label)
        
        # Create the grid widget - it will initialize itself in the constructor
        self.grid_widget = GridWidget()
        layout.addWidget(self.grid_widget)
        
        # Controls info
        controls_label = QLabel("\nControls:\n"
                               "- Arrow keys: Move selector\n"
                               "- Z/X: Change Z-level\n"
                               "- Enter/Space: Select/deselect entities\n"
                               "- Click to select/move entities")
        controls_label.setStyleSheet("color: #aaa; font-size: 12px;")
        layout.addWidget(controls_label)
        
        # Add stretch to push content up
        layout.addStretch()
        
        # Set appropriate size policy for the entire app
        from PySide6.QtWidgets import QSizePolicy
        self.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        
        # Give the widget focus to receive keyboard events
        self.grid_widget.setFocus()