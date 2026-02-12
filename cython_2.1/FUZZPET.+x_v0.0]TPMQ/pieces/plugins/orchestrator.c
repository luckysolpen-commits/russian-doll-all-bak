#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <termios.h>
#include <errno.h>
#include <signal.h>
#include <poll.h>

struct termios orig_termios;
volatile sig_atomic_t quit_flag = 0;  // Flag to signal quit

void signal_handler(int sig) {
    quit_flag = 1;
}

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
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);  // Initially disable ISIG for raw mode
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
                        case '1': return 1005; // HOME_KEY
                        case '3': return 1004; // DEL_KEY
                        case '4': return 1006; // END_KEY
                        case '5': return 1007; // PAGE_UP
                        case '6': return 1008; // PAGE_DOWN
                    }
                }
            } else {
                switch (seq[1]) {
                    case 'A': return 1000; // ARROW_UP
                    case 'B': return 1001; // ARROW_DOWN
                    case 'C': return 1002; // ARROW_RIGHT
                    case 'D': return 1003; // ARROW_LEFT
                }
            }
        } else if (seq[0] == 'O') {
            switch (seq[1]) {
                case 'H': return 1005; // HOME_KEY
                case 'F': return 1006; // END_KEY
            }
        }
        return '\x1b';
    } else {
        return c;
    }
}

// Helper function to convert command number to actual command
char* commandFromNumber(char num) {
    switch(num) {
        case '1': return "feed";
        case '2': return "play";
        case '3': return "sleep";
        case '4': return "status";
        case '5': return "evolve";
        case '6': return "quit";
        default: return NULL;
    }
}

int main() {
    printf("=== TPM Virtual Pet – FuzzPet C Version ===\n");
    printf("Commands: feed, play, sleep, status, evolve, quit\n");
    printf("Shortcuts: 1=feed, 2=play, 3=sleep, 4=status, 5=evolve, 6=quit\n");
    printf("Ctrl+C to quit\n\n");

    // Ensure directories exist
    system("mkdir -p pieces/fuzzpet pieces/master_ledger");

    // Set up raw mode for input (after printing welcome messages)
    enableRawMode();

    // Register signal handler for Ctrl+C
    signal(SIGINT, signal_handler);

    char input_buffer[50];
    int input_pos = 0;

    printf("> ");
    fflush(stdout);

    while (1) {
        // Check if quit signal was received (Ctrl+C)
        if (quit_flag) {
            printf("\rBye bye~ FuzzPet waves goodbye!\r");
            break;
        }

        int ch = readKey();

        if (ch == '\r' || ch == '\n') {  // Enter pressed
            input_buffer[input_pos] = '\0';
            input_pos = 0;

            if (strlen(input_buffer) == 0) continue;

            // Check if it's a single digit (shortcut)
            char* command = input_buffer;
            if (strlen(command) == 1) {
                command = commandFromNumber(input_buffer[0]);
                if (command == NULL) command = input_buffer;  // treat as text if not a valid number
            }

            // Simply display the command that was entered
            printf("\n> %s", input_buffer);  // Show what was entered

            // Log the command to the master ledger first
            char log_cmd[500];
            snprintf(log_cmd, sizeof(log_cmd), "./pieces/plugins/+x/log_event.+x MethodCall \"%s on fuzzpet\" \"Caller: user_input\"", command);
            system(log_cmd);
            
            // Execute the command via system() call
            if (strcmp(command, "quit") == 0) {
                printf("\nBye bye~ FuzzPet waves goodbye!\n");
                break;
            } else if (strcmp(command, "feed") == 0) {
                system("./pieces/plugins/+x/feed.+x");
                system("./pieces/plugins/+x/event_fired.+x fed");
                // Log the event firing
                system("./pieces/plugins/+x/log_event.+x EventFire \"fed on fuzzpet\" \"Trigger: feed_action\"");
            } else if (strcmp(command, "play") == 0) {
                system("./pieces/plugins/+x/play.+x");
                system("./pieces/plugins/+x/event_fired.+x played");
                // Log the event firing
                system("./pieces/plugins/+x/log_event.+x EventFire \"played on fuzzpet\" \"Trigger: play_action\"");
            } else if (strcmp(command, "sleep") == 0) {
                system("./pieces/plugins/+x/sleep.+x");
                system("./pieces/plugins/+x/event_fired.+x slept");
                // Log the event firing
                system("./pieces/plugins/+x/log_event.+x EventFire \"slept on fuzzpet\" \"Trigger: sleep_action\"");
            } else if (strcmp(command, "status") == 0) {
                system("./pieces/plugins/+x/status.+x");
            } else if (strcmp(command, "evolve") == 0) {
                system("./pieces/plugins/+x/evolve.+x");
                system("./pieces/plugins/+x/event_fired.+x evolve");
                // Log the event firing
                system("./pieces/plugins/+x/log_event.+x EventFire \"evolve on fuzzpet\" \"Trigger: evolve_action\"");
            } else {
                // Fire command_received event for any command
                system("./pieces/plugins/+x/event_fired.+x command_received");
                // Log the command received event
                system("./pieces/plugins/+x/log_event.+x EventFire \"command_received on fuzzpet\" \"Trigger: user_input\"");
                printf("\n[Pet] *tilts head* ...huh?");
            }
            
            printf("\n> ");
            fflush(stdout);
        }
        else if (ch == 127 || ch == '\b') {  // Backspace
            if (input_pos > 0) {
                input_pos--;
                printf("\b \b");
                fflush(stdout);
            }
        }
        else if (ch >= 32 && ch <= 126) {  // Printable characters
            if (input_pos < sizeof(input_buffer) - 1) {
                input_buffer[input_pos++] = ch;
                printf("%c", ch);
                fflush(stdout);
            }
        }
        else if (ch == 3) {  // Ctrl+C to quit
            printf("\rBye bye~ FuzzPet waves goodbye!\r");
            break;
        }
        else if (ch == 4) {  // Ctrl+D to quit
            printf("\rBye bye~ FuzzPet waves goodbye!\r");
            break;
        }
    }

    return 0;
}