/*** main.c - TPM Compliant Paired Keyboard System ***/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/wait.h>

// Include the modules directly since we're not using headers
#include "piece_manager.c"
#include "event_manager.c"
#include "display.c"

// Include keyboard input functionality for direct access
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <termios.h>
#include <errno.h>
#include <time.h>

#define HISTORY_FILE "peices/input/keyboard/history.txt"
#define MAX_PATH 256

struct termios orig_termios;

void die(const char *s) {
  perror(s);
  exit(1);
}

void disableRawMode() {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1)
    die("tcsetattr");
}

void enableRawMode() {
  if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) die("tcgetattr");
  atexit(disableRawMode);

  struct termios raw = orig_termios;
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= (CS8);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

int readKey() {
  int nread;
  char c;
  while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
    if (nread == -1 && errno != EAGAIN) die("read");
  }

  if (c == '\x1b') {
    char seq[3];
    if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
    if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

    if (seq[0] == '[') {
      if (seq[1] >= '0' && seq[1] <= '9') {
        if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
        if (seq[2] == '~') {
          switch (seq[1]) {
            case '1': return 1004; // HOME_KEY; 
            case '3': return 1005; // DEL_KEY;
            case '4': return 1006; // END_KEY;
            case '5': return 1007; // PAGE_UP;
            case '6': return 1008; // PAGE_DOWN;
          }
        }
      } else {
        switch (seq[1]) {
          case 'A': return 1000; // ARROW_UP;
          case 'B': return 1001; // ARROW_DOWN;
          case 'C': return 1002; // ARROW_RIGHT;
          case 'D': return 1003; // ARROW_LEFT;
        }
      }
    } else if (seq[0] == 'O') {
      switch (seq[1]) {
        case 'H': return 1004; // HOME_KEY;
        case 'F': return 1006; // END_KEY;
      }
    }
    return '\x1b';
  } else {
    return c;
  }
}

void logKey(int key) {
  FILE *fp = fopen(HISTORY_FILE, "a"); // Append mode to preserve history
  if (!fp) {
    // Create directory if it doesn't exist
    system("mkdir -p peices/input/keyboard");
    fp = fopen(HISTORY_FILE, "a");
    if (!fp) die("fopen history file");
  }
  
  char* key_str;
  char temp_str[20];
  
  // Write special representation for special keys
  if (key == 1000) key_str = "[LEFT]";
  else if (key == 1001) key_str = "[DOWN]";
  else if (key == 1002) key_str = "[RIGHT]";
  else if (key == 1003) key_str = "[UP]";
  else if (key == 13 || key == 10) key_str = "[ENTER]";
  else if (key == 127 || key == 8) key_str = "[BACKSPACE]";
  else if (key == '\t') key_str = "[TAB]";
  else if (key < 32) {
      snprintf(temp_str, sizeof(temp_str), "[CTRL_%d]", key);
      key_str = temp_str;
  } else {
      temp_str[0] = (char)key;
      temp_str[1] = '\0';
      key_str = temp_str;
  }
  
  fprintf(fp, "%s\n", key_str);
  fclose(fp);
}

// Check if history file has changed
int getHistoryLineCount(const char* history_file) {
    FILE* fp = fopen(history_file, "r");
    if (!fp) return 0;
    
    char line[256];
    int count = 0;
    while (fgets(line, sizeof(line), fp)) {
        count++;
    }
    fclose(fp);
    return count;
}

int main() {
    printf("Starting Compliant Paired Keyboard System...\n");
    printf("Using True Piece Method Architecture\n");
    printf("- Silent input mode enabled (no terminal echo)\n");
    printf("- Input logged to history.txt\n");
    printf("- Display reads from history to simulate typing\n");
    printf("- Press Ctrl+C to quit\n\n");
    
    // Initialize TPM components
    initPieceManager();
    initEventManager();
    
    // Ensure directories exist
    system("mkdir -p peices/input/keyboard");
    
    // Clear history file on startup
    FILE* fp = fopen("peices/input/keyboard/history.txt", "w");
    if (fp) {
        fclose(fp);
    }
    
    // Enable raw mode for keyboard input
    enableRawMode();
    
    // Track previous history length to only update display when needed
    int prev_history_lines = 0;
    
    while (1) {
        int key = readKey();
        
        // Exit on Ctrl+C (ASCII 3)
        if (key == 3) {
          disableRawMode();
          break;
        }
        
        // Log the key to history
        logKey(key);
        
        // Monitor keyboard piece changes
        monitorPieceChanges("input", "keyboard");
        
        // Check if history file has changed
        int curr_history_lines = getHistoryLineCount("peices/input/keyboard/history.txt");
        
        // Only update display if history has changed
        if (curr_history_lines != prev_history_lines) {
            renderFromHistory();
            prev_history_lines = curr_history_lines;
        }
    }
    
    printf("\nSystem stopped.\n");
    return 0;
}