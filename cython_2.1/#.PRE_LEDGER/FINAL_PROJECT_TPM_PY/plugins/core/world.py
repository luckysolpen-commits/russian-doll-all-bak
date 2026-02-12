"""
World plugin for ASCII Roguelike game
Loads world from text-based map files and manages game world state
"""

import os
from plugin_interface import PluginInterface


class Plugin(PluginInterface):
    def __init__(self, game_engine):
        super().__init__(game_engine)
        self.map_width = 50
        self.map_height = 20
        self.world_map = []
        self.entities = {}  # {position: entity_char}
        self.piece_manager = None
        self.world_piece_name = 'world'
        
    def initialize(self):
        """Initialize the world by loading from text-based map files and piece manager"""
        print("World plugin initialized")
        
        # Find the piece manager plugin
        for plugin in self.game_engine.plugin_manager.plugin_instances:
            if hasattr(plugin, 'get_piece_state'):
                self.piece_manager = plugin
                break
        
        # Initialize world data from piece manager if available
        if self.piece_manager:
            world_state = self.piece_manager.get_piece_state(self.world_piece_name)
            if world_state:
                self.map_width = int(world_state.get('width', 50))
                self.map_height = int(world_state.get('height', 20))
        
        # Load map from the original game's map files if they exist
        self.load_world_from_files()
        
        # Update world piece with loaded data
        if self.piece_manager:
            world_state = self.piece_manager.get_piece_state(self.world_piece_name)
            if world_state:
                world_state['width'] = self.map_width
                world_state['height'] = self.map_height
                self.piece_manager.update_piece_state(self.world_piece_name, world_state)
    
    def load_world_from_files(self):
        """Load world from the map.txt file in the world piece directory (TPM compliant)"""
        # Try to load from the world piece's map.txt file first (primary TPM location)
        map_file_path = f"games/default/pieces/{self.world_piece_name}/map.txt"
        
        if os.path.exists(map_file_path):
            try:
                with open(map_file_path, 'r') as f:
                    lines = f.readlines()
                
                # Parse the map data - read all lines that are not comments
                self.world_map = []
                
                for line in lines:
                    line = line.rstrip()
                    
                    if line.strip() and not line.startswith('#'):
                        # Only add lines that look like map data (contain symbols)
                        if any(c in line for c in ['#', '.', '~', 'T', 'R', '&', 'Z']):
                            # Ensure consistent width
                            row = line[:self.map_width].ljust(self.map_width)
                            self.world_map.append(list(row))
                
                # Trim map to specified dimensions if needed
                if len(self.world_map) > self.map_height:
                    self.world_map = self.world_map[:self.map_height]
                
                # Pad map if it's smaller than expected
                while len(self.world_map) < self.map_height:
                    row = ['.'] * self.map_width
                    self.world_map.append(row)
                
                print(f"Loaded world map from piece directory: {len(self.world_map)} rows, {len(self.world_map[0]) if self.world_map else 0} cols")
                
                # Also load entities if they exist
                self.load_entities_from_file()
                
            except Exception as e:
                print(f"Error loading map from {map_file_path}: {e}")
                # Create a default map if loading fails
                self.create_default_map()
        else:
            print(f"Map file {map_file_path} not found in piece directory, creating default map")
            self.create_default_map()
    
    def load_entities_from_file(self):
        """Load entities from the entities file in the world piece directory (TPM compliant)"""
        # Try to load from the world piece's entities.txt file first (primary TPM location)
        entities_file_path = f"games/default/pieces/{self.world_piece_name}/entities.txt"
        
        if os.path.exists(entities_file_path):
            try:
                with open(entities_file_path, 'r') as f:
                    lines = f.readlines()
                
                reading_entities = False
                for line in lines:
                    line = line.strip()
                    if line.startswith('# ENTITY DATA'):
                        reading_entities = True
                        continue
                    
                    if reading_entities and line and not line.startswith('#'):
                        parts = line.split()
                        if len(parts) >= 4:  # x y symbol type
                            try:
                                x, y = int(parts[0]), int(parts[1])
                                symbol = parts[2]
                                if 0 <= x < self.map_width and 0 <= y < self.map_height:
                                    # Store entity at position
                                    # Skip '@' symbols from entities file as they are decorative (like buildings with spawn points)
                                    # Only store non-player symbols in the entities dictionary
                                    if symbol != '@':
                                        self.entities[(y, x)] = symbol
                            except ValueError:
                                continue
                
                print(f"Loaded {len(self.entities)} entities from {entities_file_path}")
            except Exception as e:
                print(f"Error loading entities from {entities_file_path}: {e}")
        else:
            print(f"Entities file {entities_file_path} not found in piece directory")
    
    def create_default_map(self):
        """Create a default map if loading fails"""
        self.world_map = []
        for y in range(self.map_height):
            row = []
            for x in range(self.map_width):
                if x == 0 or x == self.map_width - 1 or y == 0 or y == self.map_height - 1:
                    row.append('#')  # Border walls
                elif x % 10 == 0 and y % 10 == 0:
                    row.append('T')  # Trees
                elif x % 15 == 5 and y % 15 == 5:
                    row.append('~')  # Water
                elif x % 7 == 3 and y % 7 == 3:
                    row.append('R')  # Rocks
                else:
                    row.append('.')  # Ground
            self.world_map.append(row)
    
    def get_tile(self, x, y):
        """Get the tile at position (x, y)"""
        if 0 <= y < len(self.world_map) and 0 <= x < len(self.world_map[0]):
            return self.world_map[y][x]
        return '#'
    
    def set_tile(self, x, y, tile_char):
        """Set the tile at position (x, y)"""
        if 0 <= y < len(self.world_map) and 0 <= x < len(self.world_map[0]):
            self.world_map[y][x] = tile_char
            return True
        return False
    
    def check_collision(self, x, y):
        """Check if a position has collision (impassable tile)"""
        tile = self.get_tile(x, y)
        return tile in ['#']  # Walls are impassable
    
    def get_display_map_with_entities(self):
        """Get the map for display, combining world tiles with entities"""
        # Start with the base map
        display_map = [row[:] for row in self.world_map]  # Deep copy
        
        # Clear any @ symbols that might be in the static map data
        # These should not be permanent player positions
        for y in range(len(display_map)):
            for x in range(len(display_map[0])):
                if display_map[y][x] == '@':
                    display_map[y][x] = '.'  # Replace any @ in static map with ground
        
        # Overlay entities on top of the map
        for (y, x), entity_char in self.entities.items():
            if 0 <= y < len(display_map) and 0 <= x < len(display_map[0]):
                display_map[y][x] = entity_char
        
        # Get player position from player plugin if available
        player_plugin = self.game_engine.plugin_manager.find_plugin_by_capability('get_player_position')
        if player_plugin:
            player_pos = player_plugin.get_player_position()
            if player_pos and len(player_pos) >= 2:
                px, py = player_pos[0], player_pos[1]
                if 0 <= py < len(display_map) and 0 <= px < len(display_map[0]):
                    display_map[py][px] = '@'  # Player symbol
        
        return display_map
    
    def handle_input(self, key):
        """Handle input events"""
        pass
    
    def update(self, dt):
        """Update world state"""
        pass
    
    def render(self, screen):
        """Render the world to screen"""
        # This will be called by the display plugin to get the world map
        pass