/*** keyboard_input.c - TPM Compliant Keyboard Input Handler ***/

#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>

// Define the history file path
#define HISTORY_FILE "peices/input/keyboard/history.txt"

struct termios orig_termios;

/*** terminal ***/

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

// Log key to history file
void logKey(int key) {
  FILE *fp = fopen(HISTORY_FILE, "a"); // Append mode to preserve history
  if (!fp) {
    // Create directory if it doesn't exist
    system("mkdir -p peices/input/keyboard");
    fp = fopen(HISTORY_FILE, "a");
    if (!fp) die("fopen history file");
  }
  
  // Write special representation for special keys
  if (key == 1000) fprintf(fp, "[LEFT]\n");
  else if (key == 1001) fprintf(fp, "[DOWN]\n");
  else if (key == 1002) fprintf(fp, "[RIGHT]\n");
  else if (key == 1003) fprintf(fp, "[UP]\n");
  else if (key == 13 || key == 10) fprintf(fp, "[ENTER]\n");  // Enter key
  else if (key == 127 || key == 8) fprintf(fp, "[BACKSPACE]\n");  // Backspace
  else if (key == '\t') fprintf(fp, "[TAB]\n");  // Tab
  else if (key < 32) fprintf(fp, "[CTRL_%d]\n", key);  // Control characters
  else fprintf(fp, "%c\n", key);  // Regular characters
  
  fclose(fp);
}

// Process keyboard input
void processInput() {
  enableRawMode();
  
  while (1) {
    int key = readKey();
    
    // Exit on Ctrl+C (ASCII 3)
    if (key == 3) {
      disableRawMode();
      exit(0);
    }
    
    // Log the key to history
    logKey(key);
    
    // Small delay to prevent overwhelming the system
    usleep(16667); // ~60 FPS
  }
}

// Clear the history file
void clearHistory() {
  FILE *fp = fopen(HISTORY_FILE, "w");
  if (fp) {
    fclose(fp);
  }
}

int main() {
  // Clear history on startup
  clearHistory();
  
  processInput();
  return 0;
}