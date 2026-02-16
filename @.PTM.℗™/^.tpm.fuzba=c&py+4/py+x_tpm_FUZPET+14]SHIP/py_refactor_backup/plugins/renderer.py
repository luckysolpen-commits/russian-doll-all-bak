#!/usr/bin/env python3
"""
Renderer plugin that monitors and displays content from current_frame.txt
Based on the C renderer.c implementation
"""

import os
import sys
import time
import hashlib
from datetime import datetime

script_dir = os.path.dirname(__file__)
CURRENT_FRAME_PATH = os.path.join(script_dir, '../pieces/display/current_frame.txt')

# Global variable for tracking changes
last_current_frame_time = 0


def get_file_mod_time(filepath):
    """Get the modification time of a file"""
    try:
        stat_info = os.stat(filepath)
        return stat_info.st_mtime
    except OSError:
        return 0


def render_display():
    """Render the display content from current_frame.txt"""
    # Print a separator to mark the new frame
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    print(f"\n--- FRAME UPDATE at {timestamp} ---")
    
    # Read and display the current frame from the display piece
    if os.path.exists(CURRENT_FRAME_PATH):
        try:
            with open(CURRENT_FRAME_PATH, 'r', encoding='utf-8') as frame_file:
                content = frame_file.read()
                print(content, end='')
        except IOError:
            # If we can't read the file, show default display
            show_default_display()
    else:
        # If current_frame.txt doesn't exist, create a default display
        show_default_display()


def show_default_display():
    """Show the default display content"""
    print("==================================")
    print("       FUZZPET DASHBOARD        ")
    print("==================================")
    print("Pet Name: Fuzzball")
    print("Hunger: 50")
    print("Happiness: 75")
    print("Energy: 100")
    print("Level: 1")
    print("Position: (5, 5)")
    print()
    print("GAME MAP:")
    print("####################")
    print("#  ...............T#")
    print("#  ...............T#") 
    print("#  ....R@.........T#")
    print("#  ....R..........T#")
    print("#  ....R..........T#")
    print("#  ....R..........T#")
    print("#  ................#")
    print("#                  #")
    print("####################")
    print()
    print("Controls: w/s/a/d to move, 1=feed, 2=play, 3=sleep, 4=status, 5=evolve")
    print("==================================")


def main():
    """Main function for the renderer"""
    print("Renderer Service Started")
    print("Monitoring pieces/display/current_frame.txt for changes...")
    print("Only updates display when content changes.")
    print("Preserving scroll history as frames are appended.")
    print("Press Ctrl+C to stop renderer.")
    print()

    # Ensure the display directory exists
    display_dir = os.path.join(script_dir, "../pieces/display/")
    os.makedirs(display_dir, exist_ok=True)
    
    # Create current_frame.txt if it doesn't exist
    if not os.path.exists(CURRENT_FRAME_PATH):
        with open(CURRENT_FRAME_PATH, 'w', encoding='utf-8') as test_file:
            test_file.write("==================================\n")
            test_file.write("       FUZZPET DASHBOARD        \n")
            test_file.write("==================================\n")
            test_file.write("Pet Name: Fuzzball\n")
            test_file.write("Hunger: 50\n")
            test_file.write("Happiness: 75\n")
            test_file.write("Energy: 100\n")
            test_file.write("Level: 1\n")
            test_file.write("Position: (5, 5)\n")
            test_file.write("\n")
            test_file.write("GAME MAP:\n")
            test_file.write("####################\n")
            test_file.write("#  ...............T#\n")
            test_file.write("#  ...............T#\n") 
            test_file.write("#  ....R@.........T#\n")
            test_file.write("#  ....R..........T#\n")
            test_file.write("#  ....R..........T#\n")
            test_file.write("#  ....R..........T#\n")
            test_file.write("#  ................#\n")
            test_file.write("#                  #\n")
            test_file.write("####################\n")
            test_file.write("\nControls: w/s/a/d to move, 1=feed, 2=play, 3=sleep, 4=status, 5=evolve\n")
            test_file.write("==================================\n")
    
    # Do initial render
    render_display()
    global last_current_frame_time
    last_current_frame_time = get_file_mod_time(CURRENT_FRAME_PATH)
    
    # Main monitoring loop
    try:
        while True:
            current_time = get_file_mod_time(CURRENT_FRAME_PATH)
            if current_time != last_current_frame_time:
                render_display()
                last_current_frame_time = current_time
            time.sleep(0.1)  # Check every 100ms (equivalent to usleep(100000) in C)
    except KeyboardInterrupt:
        print("\nRenderer service stopped.")


if __name__ == "__main__":
    main()