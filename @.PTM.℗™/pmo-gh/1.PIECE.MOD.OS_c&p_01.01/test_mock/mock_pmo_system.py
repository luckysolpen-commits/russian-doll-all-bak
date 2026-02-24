#!/usr/bin/env python3
"""
Mock PMO System - For Testing verify_pmo.py
This simulates a PMO system with CONTROLLABLE behavior.
"""

import os
import sys
import time
import signal

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
STATE_PATH = os.path.join(SCRIPT_DIR, "pieces/chtpm/state.txt")
HISTORY_PATH = os.path.join(SCRIPT_DIR, "pieces/keyboard/history.txt")
FRAME_PATH = os.path.join(SCRIPT_DIR, "pieces/display/current_frame.txt")

class MockPMO:
    def __init__(self, mode="perfect"):
        self.mode = mode  # "perfect", "auto_trigger_bug", "tag_leak_bug"
        self.layout = "pieces/chtpm/layouts/os.chtpm"
        self.focus_index = 0
        self.last_history_pos = 0
        
    def setup(self):
        os.makedirs(os.path.dirname(STATE_PATH), exist_ok=True)
        os.makedirs(os.path.dirname(HISTORY_PATH), exist_ok=True)
        os.makedirs(os.path.dirname(FRAME_PATH), exist_ok=True)
        # Clean start
        open(HISTORY_PATH, 'w').close()
        self.write_state()
        self.compose_frame()
        
    def write_state(self):
        with open(STATE_PATH, 'w') as f:
            f.write(f"focus_index={self.focus_index}\n")
            f.write(f"active_index=-1\n")
            f.write(f"current_layout={self.layout}\n")
    
    def compose_frame(self):
        if self.mode == "tag_leak_bug":
            # Simulate tag leakage bug
            content = "<text label=\"Hello\" />\n<button label=\"Click\" />"
        else:
            # Clean rendering
            content = "╔════════════════════════════╗\n║  CHTPM+OS Menu            ║\n╚════════════════════════════╝\n"
        with open(FRAME_PATH, 'w') as f:
            f.write(content)
    
    def process_history(self):
        if not os.path.exists(HISTORY_PATH):
            return
        with open(HISTORY_PATH, 'r') as f:
            f.seek(self.last_history_pos)
            for line in f:
                line = line.strip()
                if not line.isdigit():
                    continue
                key = int(line)
                self.handle_key(key)
            self.last_history_pos = f.tell()
    
    def handle_key(self, key):
        # Simulate auto-trigger bug if mode is set
        if self.mode == "auto_trigger_bug":
            # Bug: digit key triggers immediately
            if key == 2:
                self.layout = "pieces/chtpm/layouts/main.chtpm"  # Auto-trigger!
                self.write_state()
                self.compose_frame()
                return
        
        # Normal behavior
        if key == 2:
            # Just move focus, don't trigger
            self.focus_index = 1
            self.write_state()
        elif key == 13:  # Enter
            if self.layout.endswith("os.chtpm"):
                self.layout = "pieces/chtpm/layouts/main.chtpm"
                self.write_state()
        self.compose_frame()
    
    def run(self):
        self.setup()
        try:
            while True:
                self.process_history()
                time.sleep(0.1)
        except KeyboardInterrupt:
            pass

def main():
    mode = sys.argv[1] if len(sys.argv) > 1 else "perfect"
    print(f"[Mock PMO] Starting in '{mode}' mode", file=sys.stderr)
    mock = MockPMO(mode)
    mock.run()

if __name__ == "__main__":
    main()
