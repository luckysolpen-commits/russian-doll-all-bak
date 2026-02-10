#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <termios.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>

#define MAX_CMD 256

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

// Global to track last read position in keyboard history
static long last_read_position = 0;

// Function to read new inputs from history file
int getNewInputs(int *inputs, int max_inputs) {
    FILE *fp = fopen("peices/keyboard/history.txt", "r");
    if (!fp) return 0;
    
    // Seek to the last read position
    fseek(fp, last_read_position, SEEK_SET);
    
    int count = 0;
    char line[256];
    while (fgets(line, sizeof(line), fp) && count < max_inputs) {
        if (line[0] != '#' && strlen(line) > 1) {  // Skip comments
            int key_code;
            if (sscanf(line, "%d", &key_code) == 1) {
                inputs[count++] = key_code;
            }
        }
    }
    
    // Update the last read position
    last_read_position = ftell(fp);
    fclose(fp);
    
    return count;
}

int main() {
    printf("Starting TPM Orchestration System...\r");
    printf("Use WASD or arrow keys to move, 0-9 for menu\r");
    printf("Press Ctrl+C to quit\r\r");

    // Ensure directories and piece files exist
    system("mkdir -p peices/world peices/player peices/menu peices/keyboard");
    
    // Create default piece files if they don't exist
    if (access("peices/world/map.txt", F_OK) != 0) {
        FILE *fp = fopen("peices/world/map.txt", "w");
        if (fp) {
            fprintf(fp, "#                  #          #                  #\r");
            for (int i = 0; i < 18; i++) {
                fprintf(fp, "#..................#..................#..................#\r");
            }
            fprintf(fp, "##################################################\r");
            fclose(fp);
        }
    }
    
    if (access("peices/player/player.txt", F_OK) != 0) {
        FILE *fp = fopen("peices/player/player.txt", "w");
        if (fp) {
            fprintf(fp, "x 15\r");
            fprintf(fp, "y 10\r");
            fprintf(fp, "symbol @\r");
            fprintf(fp, "health 100\r");
            fprintf(fp, "status active\r");
            fclose(fp);
        }
    }

    // Start keyboard handler in background
    printf("Starting keyboard handler...\r");
    // Fork to run the keyboard handler
    pid_t kb_pid = fork();
    if (kb_pid == 0) {
        // Child process: run keyboard handler
        execl("./+x/keyboard_handler.+x", "keyboard_handler", NULL);
        exit(1); // If execl returns, it failed
    }
    
    // Initial display
    system("./+x/render_map.+x");

    // Main loop
    int last_player_x = -1, last_player_y = -1;
    int input_buffer[10];  // Buffer for keyboard inputs
    
    while (1) {
        // Get current player position
        char cmd[MAX_CMD];
        FILE *fp = popen("./+x/get_player_pos.+x", "r");
        int curr_x = 0, curr_y = 0;
        
        if (fp) {
            if (fscanf(fp, "%d %d", &curr_x, &curr_y) == 2) {
                // Successfully read position
            }
            pclose(fp);
        }

        // Only update display if player position changed
        if (curr_x != last_player_x || curr_y != last_player_y) {
            // Let the render_map module handle the full display since it knows how to combine all elements
            system("./+x/render_map.+x");
            
            last_player_x = curr_x;
            last_player_y = curr_y;
        }

        // Get new inputs from keyboard history
        int num_inputs = getNewInputs(input_buffer, 10);
        
        // Process each input
        for (int i = 0; i < num_inputs; i++) {
            int key = input_buffer[i];
            
            // Handle movement
            if (key == 'w' || key == 'W' || key == 1000) { // W or Up Arrow
                // Get current position
                fp = popen("./+x/get_player_pos.+x", "r");
                int x = 0, y = 0;
                if (fp && fscanf(fp, "%d %d", &x, &y) == 2) {
                    pclose(fp);
                    y--; // Move up
                    snprintf(cmd, MAX_CMD, "./+x/update_player_pos.+x %d %d", x, y);
                    system(cmd);
                } else if (fp) {
                    pclose(fp);
                }
            } else if (key == 's' || key == 'S' || key == 1001) { // S or Down Arrow
                fp = popen("./+x/get_player_pos.+x", "r");
                int x = 0, y = 0;
                if (fp && fscanf(fp, "%d %d", &x, &y) == 2) {
                    pclose(fp);
                    y++; // Move down
                    snprintf(cmd, MAX_CMD, "./+x/update_player_pos.+x %d %d", x, y);
                    system(cmd);
                } else if (fp) {
                    pclose(fp);
                }
            } else if (key == 'a' || key == 'A' || key == 1003) { // A or Left Arrow
                fp = popen("./+x/get_player_pos.+x", "r");
                int x = 0, y = 0;
                if (fp && fscanf(fp, "%d %d", &x, &y) == 2) {
                    pclose(fp);
                    x--; // Move left
                    snprintf(cmd, MAX_CMD, "./+x/update_player_pos.+x %d %d", x, y);
                    system(cmd);
                } else if (fp) {
                    pclose(fp);
                }
            } else if (key == 'd' || key == 'D' || key == 1002) { // D or Right Arrow
                fp = popen("./+x/get_player_pos.+x", "r");
                int x = 0, y = 0;
                if (fp && fscanf(fp, "%d %d", &x, &y) == 2) {
                    pclose(fp);
                    x++; // Move right
                    snprintf(cmd, MAX_CMD, "./+x/update_player_pos.+x %d %d", x, y);
                    system(cmd);
                } else if (fp) {
                    pclose(fp);
                }
            } else if (key == 3) { // Ctrl+C
                // Kill the keyboard handler process
                kill(kb_pid, SIGTERM);
                exit(0);
            } else if (key >= '0' && key <= '9') { // Menu options
                printf("\rMenu option %c selected\r", (char)key);
            }
        }
        
        // Small delay to prevent excessive CPU usage
        usleep(50000); // 50ms delay
    }

    printf("\rTPM Orchestration System shutting down...\r");
    return 0;
}