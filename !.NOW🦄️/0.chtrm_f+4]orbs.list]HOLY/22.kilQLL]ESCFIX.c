/*** includes ***/

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

/*** defines ***/

#define CTRL_KEY(k) ((k) & 0x1f)

/*** key codes ***/

enum editorKey {
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    DEL_KEY,
    HOME_KEY,
    END_KEY,
    PAGE_UP,
    PAGE_DOWN,
    ESC_KEY                     // ← dedicated Escape key
};

/*** data ***/

struct termios orig_termios;

/*** terminal ***/

void die(const char *s) {
    // Don't clear screen or manipulate terminal, just exit
    perror(s);
    exit(1);
}

void restoreTerminalMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void setupNonCanonicalMode() {
    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) die("tcgetattr");
    
    struct termios new_termios = orig_termios;
    // Only disable ICANON to read input character by character
    // Keep ECHO enabled so other programs can handle display
    new_termios.c_lflag &= ~(ICANON);  // Disable canonical mode
    new_termios.c_lflag |= ECHO;       // Keep echo enabled so user sees their typing
    new_termios.c_cc[VMIN] = 1;        // Minimum characters to read
    new_termios.c_cc[VTIME] = 0;       // No timeout

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &new_termios) == -1) die("tcsetattr");
    
    // Restore original settings when exiting
    atexit(restoreTerminalMode);
}

/*** input ***/

int editorReadKey() {
    int nread;
    char c;
    
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) die("read");
    }

    if (c == '\x1b') {                  // Escape sequence starts
        char seq[3];

        // Use non-blocking read for the next two characters
        int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

        int bytes_read = read(STDIN_FILENO, &seq[0], 1);
        if (bytes_read <= 0) {
            fcntl(STDIN_FILENO, F_SETFL, flags); // Restore blocking
            return ESC_KEY; // Just an escape key
        }
        
        bytes_read = read(STDIN_FILENO, &seq[1], 1);
        if (bytes_read <= 0) {
            fcntl(STDIN_FILENO, F_SETFL, flags); // Restore blocking
            return ESC_KEY; // Just an escape key
        }

        // Restore blocking mode
        fcntl(STDIN_FILENO, F_SETFL, flags);

        if (seq[0] == '[') {
            if (seq[1] >= '0' && seq[1] <= '9') {
                if (read(STDIN_FILENO, &seq[2], 1) != 1) return ESC_KEY;
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

        return ESC_KEY;                 // Any other escape sequence → treat as Esc
    }

    return c;
}

/*** key processing ***/

void editorProcessKeypress() {
    int key = editorReadKey();

    // ── Logging (simple numeric code only) ──
    FILE *logFile = fopen("log.txt", "a");
    if (logFile) {
        // Write only simple numeric code
        fprintf(logFile, "%d\n", key);
        fclose(logFile);
    }

    // ── Quit on Ctrl-Q or Ctrl-C only (NOT ESC) ──
    if (key == CTRL_KEY('q') || key == CTRL_KEY('c')) {
        exit(0); // Just exit without terminal manipulation
    }
}

/*** init ***/

int main() {
    setupNonCanonicalMode();

    while (1) {
        editorProcessKeypress();
    }

    return 0;
}
