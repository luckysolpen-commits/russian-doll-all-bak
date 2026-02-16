#!/usr/bin/env python3
"""
Module for managing FuzzPet state and logic.
This represents the FuzzPet as a separate piece in the TPM architecture.
"""

import os
import sys
import time

# Import the PieceManager
from piece_manager import piece_manager


class FuzzPet:
    def __init__(self):
        """Initialize the FuzzPet with default state or load from file."""
        self.pet_state = {
            'pos_x': 5,
            'pos_y': 5,
            'name': 'FuzzPet',
            'hunger': 50,
            'happiness': 50,
            'energy': 50,
            'level': 1,
        }
        
        # Load state from file if it exists using PieceManager
        loaded_state = piece_manager.read_piece_state('fuzzpet')
        if loaded_state:
            self.pet_state.update(loaded_state)
        else:
            # Save initial state if it didn't exist
            self.save_state()
    
    def load_state(self):
        """Load the FuzzPet state from the piece.txt file using PieceManager."""
        loaded_state = piece_manager.read_piece_state('fuzzpet')
        if loaded_state:
            self.pet_state.update(loaded_state)
    
    def save_state(self):
        """Save the current FuzzPet state to the piece.txt file using PieceManager."""
        success = piece_manager.write_piece_state('fuzzpet', self.pet_state)
        if not success:
            print("Failed to save FuzzPet state", file=sys.stderr)
    
    def get_state(self):
        """Return the current FuzzPet state."""
        return self.pet_state.copy()
    
    def update_position(self, direction):
        """Update the position based on the direction."""
        if direction == 'up':
            self.pet_state['pos_y'] = max(0, self.pet_state['pos_y'] - 1)
        elif direction == 'down':
            self.pet_state['pos_y'] += 1
        elif direction == 'left':
            self.pet_state['pos_x'] = max(0, self.pet_state['pos_x'] - 1)
        elif direction == 'right':
            self.pet_state['pos_x'] += 1
        # Save state after updating position
        self.save_state()
    
    def process_command(self, command_char):
        """Process a command character and update the FuzzPet state."""
        if command_char == '1':  # Feed
            self.pet_state['hunger'] = max(0, self.pet_state['hunger'] - 20)
            result = "[CMD] Fed the pet. Hunger decreased."
        elif command_char == '2':  # Play
            self.pet_state['happiness'] = min(100, self.pet_state['happiness'] + 15)
            result = "[CMD] Played with the pet. Happiness increased."
        elif command_char == '3':  # Sleep
            self.pet_state['energy'] = min(100, self.pet_state['energy'] + 20)
            result = "[CMD] Pet is sleeping. Energy increased."
        elif command_char == '4':  # Status
            result = "[CMD] Displaying current pet status."
        elif command_char == '5':  # Evolve
            self.pet_state['level'] += 1
            result = "[CMD] Pet has evolved! Level up!"
        else:
            result = f"[CMD] Unknown command: {command_char}"
        
        # Save state after processing command
        self.save_state()
        return result
    
    def get_status_message(self):
        """Get a formatted string with the pet's current status."""
        return f"Pet Status: Name: {self.pet_state['name']}, Hunger: {self.pet_state['hunger']}, " \
               f"Happiness: {self.pet_state['happiness']}, Energy: {self.pet_state['energy']}, " \
               f"Level: {self.pet_state['level']}"


def main():
    """Main function for the FuzzPet piece."""
    print("Starting FuzzPet piece.", file=sys.stderr)
    fuzzpet = FuzzPet()
    
    # Keep the process alive to maintain the FuzzPet state
    try:
        while True:
            # The FuzzPet piece runs as a separate process but mainly serves as a state manager
            # The actual command processing will happen in conjunction with other pieces
            time.sleep(1)
    except KeyboardInterrupt:
        print("\nFuzzPet piece stopped.", file=sys.stderr)


if __name__ == "__main__":
    main()