"""
Display plugin for the Pet Philosophy project
Renders the pet position map and status following TPM principles
"""

from plugin_interface import PluginInterface


class Plugin(PluginInterface):
    def __init__(self, game_engine):
        super().__init__(game_engine)
        self.piece_manager = None
        self.fuzzpet_piece_name = 'fuzzpet'
        self.world_piece_name = 'world'
        self.display_piece_name = 'display'
        self.map_file_path = 'pieces/world/map.txt'
        self.frame_history_path = 'pieces/display/frame_history.txt'
        self.current_frame_path = 'pieces/display/current_frame.txt'
        
    def initialize(self):
        """Initialize display plugin and display piece"""
        print("Display plugin initialized")
        
        # Find the piece manager plugin
        for plugin in self.game_engine.plugin_manager.plugin_instances:
            if hasattr(plugin, 'get_piece_state'):
                self.piece_manager = plugin
                break
        
        # Initialize the display piece with frame history file
        if self.piece_manager:
            # Create display piece directory and files if they don't exist
            import os
            display_dir = os.path.join("pieces", "display")
            os.makedirs(display_dir, exist_ok=True)
            
            # Create current_frame.txt if it doesn't exist
            if not os.path.exists(self.current_frame_path):
                with open(self.current_frame_path, 'w') as f:
                    f.write("")
            
            # Create frame_history.txt if it doesn't exist
            if not os.path.exists(self.frame_history_path):
                with open(self.frame_history_path, 'w') as f:
                    f.write("# Frame History - Latest frame at end\n")
            
            # Create a display piece if it doesn't exist
            display_state = self.piece_manager.get_piece_state(self.display_piece_name)
            if not display_state:
                display_state = {
                    'current_frame': '',
                    'frame_count': 0,
                    'status': 'active',
                    'methods': ['render_frame', 'clear_frame', 'save_frame'],
                    'event_listeners': ['frame_requested', 'display_update']
                }
                self.piece_manager.update_piece_state(self.display_piece_name, display_state)
    
    def render_frame_to_file(self):
        """Render the frame to a string and save to file"""
        if not self.piece_manager:
            return ""
            
        # Get fuzzpet state for position
        fuzzpet_state = self.piece_manager.get_piece_state(self.fuzzpet_piece_name)
        if not fuzzpet_state:
            return ""
            
        # Get pet coordinates from state (add them if they don't exist)
        # Default position should be within map bounds to prevent errors
        try:
            pet_x = max(0, int(fuzzpet_state.get('state', {}).get('pos_x', 5)))
            pet_y = max(0, int(fuzzpet_state.get('state', {}).get('pos_y', 5)))
        except (ValueError, TypeError):
            pet_x, pet_y = 5, 5  # default position
        
        # Ensure the pet coordinates are within map boundaries
        # Load the map file to check dimensions
        try:
            with open(self.map_file_path, 'r') as f:
                map_lines = f.readlines()
        except FileNotFoundError:
            # Create a simple 10x20 map for fallback
            map_lines = [
                "##########\n",
                "#        #\n",
                "#        #\n",
                "#        #\n",
                "#        #\n",
                "#        #\n",
                "#        #\n",
                "#        #\n",
                "#        #\n",
                "##########\n"
            ]
        
        # Convert map to list of lists for manipulation
        map_grid = [list(line.rstrip('\n')) for line in map_lines]
        
        # Make sure coordinates are within valid range
        max_y = len(map_grid) - 1
        max_x = 0
        if max_y >= 0:
            max_x = len(map_grid[0]) - 1 if len(map_grid[0]) > 0 else 0
            pet_x = min(pet_x, max_x)
            pet_y = min(pet_y, max_y)
        
        # Place the pet on the map (using '@' for pet)
        if 0 <= pet_y < len(map_grid) and 0 <= pet_x < len(map_grid[0]):
            # Don't overwrite important map features, but note we could refine this
            if map_grid[pet_y][pet_x] not in ['#', 'T', 'R', '~', 'Z']:  # Avoid overwriting important features
                map_grid[pet_y][pet_x] = '@'  # Use @ to represent the pet
        
        # Build the frame as a string
        frame_content = []
        frame_content.append("="*60)
        frame_content.append("PET POSITION MAP:")
        frame_content.append("="*60)
        
        # Add the map
        for line in map_grid:
            frame_content.append(''.join(line))
        
        frame_content.append(f"Pet Position: ({pet_x}, {pet_y})")
        frame_content.append("="*60)
        
        # Show pet status as well
        state = fuzzpet_state.get('state', {})
        frame_content.append(f"Pet Status: Name: {state.get('name', 'Unknown')}, Hunger: {state.get('hunger', 'Unknown')}, "
                           f"Happiness: {state.get('happiness', 'Unknown')}, Energy: {state.get('energy', 'Unknown')}, "
                           f"Level: {state.get('level', 'Unknown')}")
        
        # Show any recent command output if available
        # Find the command processor plugin to get recent output
        command_processor = self.game_engine.plugin_manager.find_plugin_by_capability('last_command_output')
        if command_processor and hasattr(command_processor, 'last_command_output') and command_processor.last_command_output:
            frame_content.append("")
            for output_line in command_processor.last_command_output:
                frame_content.append(output_line)
        
        # Show condensed commands on a single line
        available_methods = fuzzpet_state.get('methods', [])
        movement_commands = "Movement: W↑|A←|S↓|D→"
        
        # Build pet commands in a condensed format
        if available_methods:
            pet_commands = "Pet Commands: " + "|".join([f"{idx}[{method}]" for idx, method in enumerate(available_methods, 1)])
        else:
            pet_commands = "Pet Commands: None"
        
        frame_content.append(f"\n{movement_commands} | {pet_commands}")
        
        # Join and return the complete frame
        complete_frame = "\n".join(frame_content)
        return complete_frame
    
    def update_display(self):
        """Render the frame and update the display by writing to file and showing the file content"""
        # Render the current frame to a string
        frame_content = self.render_frame_to_file()
        
        # Write the complete frame to the current_frame file
        with open(self.current_frame_path, 'w') as f:
            f.write(frame_content)
        
        # Write to frame history as well (append)
        import time
        timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
        with open(self.frame_history_path, 'a') as f:
            f.write(f"\n--- FRAME RENDERED AT {timestamp} ---\n{frame_content}\n--- END FRAME ---\n")
        
        # Update the display piece state
        display_state = self.piece_manager.get_piece_state(self.display_piece_name)
        if display_state:
            import time
            display_state['current_frame'] = f"Last updated: {timestamp}"
            display_state['frame_count'] = int(display_state.get('frame_count', 0)) + 1
            self.piece_manager.update_piece_state(self.display_piece_name, display_state)
        
        # Clear the screen and display the content from the file
        print("\033[2J\033[H", end="")  # ANSI codes to clear screen and move cursor to top-left
        
        # Read and print the content from the current frame file
        with open(self.current_frame_path, 'r') as f:
            print(f.read())