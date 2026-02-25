# This piece is responsible for capturing keyboard input and logging it.
# It runs as a standalone process.
# Matches termios settings from the C version keyboard_input.c

import sys
import tty
import termios
import os
import time
from datetime import datetime

# Define constants to match the C version
CTRL_KEY = lambda k: ord(k) & 0x1f
BACKSPACE = 127
ARROW_LEFT = 1000
ARROW_RIGHT = 1001
ARROW_UP = 1002
ARROW_DOWN = 1003
DEL_KEY = 1004
HOME_KEY = 1005
END_KEY = 1006
PAGE_UP = 1007
PAGE_DOWN = 1008

script_dir = os.path.dirname(__file__)
HISTORY_FILE_PATH = os.path.join(script_dir, '../pieces/keyboard/history.txt')
MASTER_LEDGER_PATH = os.path.join(script_dir, '../pieces/master_ledger/master_ledger.txt')

def setup_raw_mode():
    """Set up raw mode terminal settings to match the C version"""
    fd = sys.stdin.fileno()
    old_settings = termios.tcgetattr(fd)
    
    # Create new settings matching the C version
    new_settings = old_settings[:]
    
    # c_iflag: Disable break on INTR, CR-to-NL translation, Parity check, strip high bit, Disable ^S/^Q
    new_settings[0] &= ~(termios.BRKINT | termios.ICRNL | termios.INPCK | termios.ISTRIP | termios.IXON)
    
    # c_cflag: Set to CS8 (8 bits per byte)
    new_settings[2] |= termios.CS8
    
    # c_lflag: Disable echo, canonical mode (line buffering), extended input processing, signals
    new_settings[3] &= ~(termios.ECHO | termios.ICANON | termios.IEXTEN | termios.ISIG)
    
    # c_cc: Set VMIN to 0 (non-blocking read) and VTIME to 1 (timeout for partial reads)
    new_settings[6][termios.VMIN] = 0
    new_settings[6][termios.VTIME] = 1
    
    termios.tcsetattr(fd, termios.TCSAFLUSH, new_settings)
    
    return old_settings

def restore_terminal(settings):
    """Restore original terminal settings"""
    fd = sys.stdin.fileno()
    termios.tcsetattr(fd, termios.TCSAFLUSH, settings)

def read_key():
    """Read a keypress with arrow key and escape sequence handling"""
    fd = sys.stdin.fileno()
    
    # Wait for character to be available
    while True:
        ch = sys.stdin.read(1)
        if ch:
            break
            
    if ord(ch) == 27:  # ESC character
        # Try to read next two characters for arrow keys/escape sequences
        seq1 = sys.stdin.read(1)
        if not seq1:
            return 27  # Just return ESC if no more chars
        
        seq2 = sys.stdin.read(1)
        if not seq2:
            return 27  # Just return ESC if no more chars
        
        if seq1 == '[':
            if seq2 >= '0' and seq2 <= '9':
                seq3 = sys.stdin.read(1)
                if not seq3 or seq3 != '~':
                    return 27  # Return ESC if invalid sequence
                
                # Handle function keys and special keys
                if seq2 == '1':
                    return HOME_KEY
                elif seq2 == '3':
                    return DEL_KEY
                elif seq2 == '4':
                    return END_KEY
                elif seq2 == '5':
                    return PAGE_UP
                elif seq2 == '6':
                    return PAGE_DOWN
                elif seq2 == '7':
                    return HOME_KEY
                elif seq2 == '8':
                    return END_KEY
            else:
                # Handle arrow keys and other bracketed sequences
                if seq2 == 'A':
                    return ARROW_UP
                elif seq2 == 'B':
                    return ARROW_DOWN
                elif seq2 == 'C':
                    return ARROW_RIGHT
                elif seq2 == 'D':
                    return ARROW_LEFT
                elif seq2 == 'H':
                    return HOME_KEY
                elif seq2 == 'F':
                    return END_KEY
        elif seq1 == 'O':
            # Handle O sequences
            if seq2 == 'H':
                return HOME_KEY
            elif seq2 == 'F':
                return END_KEY
        
        # If we got here, return ESC
        return 27
    else:
        return ord(ch)

def write_command(key):
    """Write to pieces/keyboard/history.txt as per TPM standards"""
    # Ensure directory exists
    os.makedirs(os.path.dirname(HISTORY_FILE_PATH), exist_ok=True)
    
    # Write to keyboard history (just ASCII value like before)
    with open(HISTORY_FILE_PATH, 'a', encoding='utf-8') as fp:
        fp.write(f"{key}\n")
    
    # Also log to master ledger for complete audit trail
    with open(MASTER_LEDGER_PATH, 'a', encoding='utf-8') as ledger:
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        ledger.write(f"[{timestamp}] InputReceived: key_code={key} | Source: keyboard_input_service\n")

def main():
    """Main loop to capture keys and log them."""
    print("Starting keyboard piece. Press Ctrl+C to exit.", file=sys.stderr)
    
    # Save original settings and set up raw mode
    original_settings = setup_raw_mode()
    
    try:
        while True:
            key = read_key()
            
            # Check for Ctrl+C (ASCII 3)
            if key == CTRL_KEY('c'):
                print("\nCtrl+C pressed. Exiting...", file=sys.stderr)
                break
            
            write_command(key)
            
            # Sleep ~16.667ms to match ~60fps in C version
            time.sleep(16.667 / 1000)
    
    except KeyboardInterrupt:
        print("\nKeyboard piece stopped by interrupt.", file=sys.stderr)
    finally:
        # Restore original terminal settings
        restore_terminal(original_settings)

if __name__ == "__main__":
    main()
