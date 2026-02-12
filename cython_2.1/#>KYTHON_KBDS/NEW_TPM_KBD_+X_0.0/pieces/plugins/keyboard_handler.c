#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <time.h>

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
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

int kbhit() {
    fd_set rfds;
    struct timeval tv = {0, 0};
    
    FD_ZERO(&rfds);
    FD_SET(STDIN_FILENO, &rfds);
    
    return select(STDIN_FILENO + 1, &rfds, NULL, NULL, &tv) > 0;
}

int main() {
    printf("Keyboard Input Handler Started\n");
    printf("Press WASD keys for movement, q to quit\n");
    printf("All keystrokes will be logged to pieces/keyboard/history.txt\n\n");
    
    // Read current session_id and keys_recorded from keyboard piece
    char session_id_str[20];
    char keys_recorded_str[10];
    
    FILE *fp = popen("./pieces/plugins/+x/piece_manager.+x keyboard get-state session_id", "r");
    if (fp) {
        fgets(session_id_str, sizeof(session_id_str), fp);
        session_id_str[strcspn(session_id_str, "\n")] = '\0';
        pclose(fp);
    }
    
    fp = popen("./pieces/plugins/+x/piece_manager.+x keyboard get-state keys_recorded", "r");
    if (fp) {
        fgets(keys_recorded_str, sizeof(keys_recorded_str), fp);
        keys_recorded_str[strcspn(keys_recorded_str, "\n")] = '\0';
        pclose(fp);
    }
    
    int session_id = atoi(session_id_str);
    int keys_recorded = atoi(keys_recorded_str);
    
    // Set up raw mode for input
    enableRawMode();
    
    while (1) {
        if (kbhit()) {
            int ch = getchar();
            
            if (ch == 'q' || ch == 'Q') {
                printf("\nQuitting keyboard handler.\n");
                break;
            }
            
            // Log key to history file
            FILE *history = fopen("pieces/keyboard/history.txt", "a");
            if (history) {
                time_t rawtime;
                struct tm *timeinfo;
                char timestamp[80];
                
                time(&rawtime);
                timeinfo = localtime(&rawtime);
                strftime(timestamp, sizeof(timestamp), "%H:%M:%S", timeinfo);
                
                fprintf(history, "%d # Key: %c, Time: %s, Session: %d\n", ch, ch, timestamp, session_id);
                fclose(history);
            }
            
            // Update keys_recorded in keyboard piece
            keys_recorded++;
            char cmd[200];
            sprintf(cmd, "./pieces/plugins/+x/piece_manager.+x keyboard set-state keys_recorded %d", keys_recorded);
            system(cmd);
            
            // Update last_key_code in keyboard piece
            sprintf(cmd, "./pieces/plugins/+x/piece_manager.+x keyboard set-state last_key_code %d", ch);
            system(cmd);
            
            // Log to master ledger
            char time_str[80];
            time_t rawtime;
            struct tm *timeinfo;
            time(&rawtime);
            timeinfo = localtime(&rawtime);
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", timeinfo);
            
            sprintf(cmd, "./pieces/plugins/+x/log_event.+x KeyEvent \"Key: (%c) %d\" \"Source: keyboard, Session: %d\"", 
                    (ch >= 32 && ch <= 126) ? ch : '?', ch, session_id);
            system(cmd);
            
            // Handle specific movement keys
            if (ch == 'w' || ch == 'W') {
                printf("W pressed - Moving up\n");
                system("./pieces/plugins/+x/move_up.+x");
                // Fire event
                system("./pieces/plugins/+x/event_fired.+x key_w");
            } else if (ch == 'a' || ch == 'A') {
                printf("A pressed - Moving left\n");
                system("./pieces/plugins/+x/move_left.+x");
                // Fire event
                system("./pieces/plugins/+x/event_fired.+x key_a");
            } else if (ch == 's' || ch == 'S') {
                printf("S pressed - Moving down\n");
                system("./pieces/plugins/+x/move_down.+x");
                // Fire event
                system("./pieces/plugins/+x/event_fired.+x key_s");
            } else if (ch == 'd' || ch == 'D') {
                printf("D pressed - Moving right\n");
                system("./pieces/plugins/+x/move_right.+x");
                // Fire event
                system("./pieces/plugins/+x/event_fired.+x key_d");
            } else {
                printf("Key pressed: %c (%d)\n", ch, ch);
            }
        }
        
        // Small delay to prevent excessive CPU usage
        usleep(50000); // 50ms
    }
    
    return 0;
}