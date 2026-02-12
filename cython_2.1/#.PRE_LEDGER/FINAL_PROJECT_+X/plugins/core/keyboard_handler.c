#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

#define MAX_PATH 256
#define COMMAND_FILE "peices/keyboard/history.txt"

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
            case '1': return 1005; // HOME_KEY;
            case '3': return 1004; // DEL_KEY;
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
        case 'H': return 1005; // HOME_KEY;
        case 'F': return 1006; // END_KEY;
      }
    }
    return '\x1b';
  } else {
    return c;
  }
}

void writeCommand(int key) {
  // Create directories if they don't exist
  system("mkdir -p peices/keyboard");
  
  FILE *fp = fopen(COMMAND_FILE, "a"); // Append mode
  if (!fp) die("fopen command file");
  fprintf(fp, "%d\n", key);
  fclose(fp);
}

int main() {
  // Create the keyboard piece directory and file if they don't exist
  system("mkdir -p peices/keyboard");
  
  // Create keyboard piece file
  FILE *kb_fp = fopen("peices/keyboard/keyboard.txt", "w");
  if (kb_fp) {
    fprintf(kb_fp, "session_id 12345\n");
    fprintf(kb_fp, "current_key_log_path %s\n", COMMAND_FILE);
    fprintf(kb_fp, "logging_enabled true\n");
    fprintf(kb_fp, "keys_recorded 0\n");
    fprintf(kb_fp, "event_listeners key_press\n");
    fprintf(kb_fp, "last_key_logged \n");
    fprintf(kb_fp, "last_key_time \n");
    fprintf(kb_fp, "last_key_code 0\n");
    fprintf(kb_fp, "methods log_key,reset_session,get_session_log,enable_logging,disable_logging\n");
    fprintf(kb_fp, "status active\n");
    fclose(kb_fp);
  }
  
  // Clear history file on startup
  FILE *fp = fopen(COMMAND_FILE, "w");
  if (fp) {
    fprintf(fp, "# Keyboard session started at runtime\n");
    fprintf(fp, "# Format: key_code (numeric)\n");
    fclose(fp);
  }

  enableRawMode();

  while (1) {
    int c = readKey();
    if (c == 3) {  // Check for Ctrl+C
      disableRawMode();        // Restore terminal settings
      exit(0);                 // Exit cleanly
    }
    writeCommand(c);
    usleep(16667); // 10ms delay to avoid overwhelming the system
  }

  return 0;
}