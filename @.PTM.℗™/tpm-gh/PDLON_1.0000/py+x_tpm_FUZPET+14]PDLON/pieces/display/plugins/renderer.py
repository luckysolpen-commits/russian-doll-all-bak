#!/usr/bin/env python3
"""
Renderer plugin - monitors frame_changed.txt marker file for reliable updates
Based on proven C renderer.c implementation
"""

import os
import sys
import time
from datetime import datetime

script_dir = os.path.dirname(__file__)
CURRENT_FRAME_PATH = os.path.join(script_dir, '../../display/current_frame.txt')
MARKER_FILE_PATH = os.path.join(script_dir, '../../master_ledger/frame_changed.txt')

# Track marker file SIZE (not mtime!)
last_marker_size = 0




def render_display():
    """Render the display content from current_frame.txt"""
    if os.path.exists(CURRENT_FRAME_PATH):
        try:
            with open(CURRENT_FRAME_PATH, 'r', encoding='utf-8') as frame_file:
                content = frame_file.read()
                print(content, end='')
                sys.stdout.flush()  # Force flush for immediate display
        except IOError:
            show_default_display()
    else:
        show_default_display()


def show_default_display():
    """Show default display content"""
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
    """Main loop - check marker file SIZE every 16.667ms (60 FPS)"""
    global last_marker_size
    
    # Renderer started
    
    # Ensure the display directory exists
    display_dir = os.path.join(script_dir, '../../')
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
    last_marker_size = os.path.getsize(MARKER_FILE_PATH) if os.path.exists(MARKER_FILE_PATH) else 0
    
    # Main monitoring loop - 60 FPS
    while True:
        try:
            # Check marker file SIZE (reliable, no mtime issues!)
            if os.path.exists(MARKER_FILE_PATH):
                current_size = os.path.getsize(MARKER_FILE_PATH)
                
                if current_size != last_marker_size:
                    # Frame was updated!
                    render_display()
                    last_marker_size = current_size
            
            # Sleep 16.667ms for 60 FPS
            time.sleep(0.016667)
            
        except BrokenPipeError:
            # Expected when parent process dies - just exit cleanly
            break
        except Exception as e:
            time.sleep(0.1)


if __name__ == "__main__":
    main()
