/*** control.c - TPM Compliant Control Module ***/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

#define MAP_WIDTH 50
#define MAP_HEIGHT 20
#define MAX_ENTITIES 100

#define CTRL_KEY(k) ((k) & 0x1f)

enum editorKey {
  BACKSPACE = 127,
  ARROW_LEFT = 1000,
  ARROW_RIGHT,
  ARROW_UP,
  ARROW_DOWN,
  DEL_KEY,
  HOME_KEY,
  END_KEY,
  PAGE_UP,
  PAGE_DOWN
};

struct termios orig_termios;

// Terminal functions
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
            case '1': return HOME_KEY;
            case '3': return DEL_KEY;
            case '4': return END_KEY;
            case '5': return PAGE_UP;
            case '6': return PAGE_DOWN;
            case '7': return HOME_KEY;
            case '8': return END_KEY;
          }
        }
      } else {
        switch (seq[1]) {
          case 'A': return ARROW_UP;
          case 'B': return ARROW_DOWN;
          case 'C': return ARROW_RIGHT;
          case 'D': return ARROW_LEFT;
          case 'H': return HOME_KEY;
          case 'F': return END_KEY;
        }
      }
    } else if (seq[0] == 'O') {
      switch (seq[1]) {
        case 'H': return HOME_KEY;
        case 'F': return END_KEY;
      }
    }
    return '\x1b';
  } else {
    return c;
  }
}

extern int player_x, player_y;

// Process control input
int processInput() {
    int key = readKey();
    
    // Handle WASD movement
    switch (key) {
        case 'w':
        case 'W':
            return movePlayer(0, -1);  // Move up
        case 's':
        case 'S':
            return movePlayer(0, 1);   // Move down
        case 'a':
        case 'A':
            return movePlayer(-1, 0);  // Move left
        case 'd':
        case 'D':
            return movePlayer(1, 0);   // Move right
        // Handle arrow keys
        case ARROW_UP:
            return movePlayer(0, -1);
        case ARROW_DOWN:
            return movePlayer(0, 1);
        case ARROW_LEFT:
            return movePlayer(-1, 0);
        case ARROW_RIGHT:
            return movePlayer(1, 0);
        // Handle menu options (0-9)
        case '0': 
            printf("Menu option 0 selected: end_turn\n");
            return 0;
        case '1': 
            printf("Menu option 1 selected: hunger\n");
            return 0;
        case '2': 
            printf("Menu option 2 selected: state\n");
            return 0;
        case '3': 
            printf("Menu option 3 selected: act\n");
            return 0;
        case '4': 
            printf("Menu option 4 selected: inventory\n");
            return 0;
        case '5': 
            printf("Menu option 5 selected\n");
            return 0;
        case '6': 
            printf("Menu option 6 selected\n");
            return 0;
        case '7': 
            printf("Menu option 7 selected\n");
            return 0;
        case '8': 
            printf("Menu option 8 selected\n");
            return 0;
        case '9': 
            printf("Menu option 9 selected\n");
            return 0;
        case CTRL_KEY('c'):  // Ctrl+C to quit
            return -1;  // Return -1 to indicate quit
        default:
            break;
    }
    
    return 0;  // No change
}

// Run the main game loop
void runGameLoop() {
    enableRawMode();
    
    extern void updateDisplay();
    updateDisplay();  // Initial render
    
    int last_player_x = player_x;
    int last_player_y = player_y;
    
    while (1) {
        int changed = processInput();
        
        if (changed == -1) {  // Quit
            break;
        }
        
        // Update display if player moved
        if (player_x != last_player_x || player_y != last_player_y) {
            updateDisplay();
            last_player_x = player_x;
            last_player_y = player_y;
        }
        
        // Small delay to prevent excessive CPU usage
        usleep(16667); // ~60 FPS
    }
    
    disableRawMode();
}