#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <termios.h>

// --- Function Prototypes ---
int check_terminal_input();
void process_input(int ch);
void print_main_menu();
void run_module(const char* module_name);

// --- Globals ---
struct termios original_termios;

// --- Main Menu Display ---
void print_main_menu() {
    printf("\033[H\033[J"); // Clear terminal
    printf("=== HydrGn Modular System ===\n");
    printf("-----------------------------\n");
    printf("0. Canvas\n");
    printf("1. Emoji Palette\n");
    printf("2. Color Palette\n");
    printf("3. Tool Selection\n");
    printf("4. Maps\n");
    printf("5. Events\n");
    printf("6. Player\n");
    printf("-----------------------------\n");
    printf("Select an option (0-6) or Q to quit: ");
}

// --- Terminal Input Handling ---
int check_terminal_input() {
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    fd_set set;
    struct timeval timeout;
    FD_ZERO(&set);
    FD_SET(STDIN_FILENO, &set);
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    int ready = select(STDIN_FILENO + 1, &set, NULL, NULL, &timeout);
    int ch = -1;
    if (ready > 0) {
        ch = getchar();
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

// --- Module Execution ---
void run_module(const char* module_name) {
    char command[256];
    snprintf(command, sizeof(command), "hydrgn.modules/+x/%s.+x", module_name);
    
    int result = system(command);
    if (result == -1) {
        printf("Error: Could not run %s module\n", module_name);
        sleep(2); // Brief pause to show error
    }
}

// --- Input Processing ---
void process_input(int ch) {
    switch (ch) {
        case '0':
            run_module("canvas");
            break;
        case '1':
            run_module("emoji_palette");
            break;
        case '2':
            run_module("color_palette");
            break;
        case '3':
            run_module("tool");
            break;
        case '4':
            run_module("maps");
            break;
        case '5':
            run_module("events");
            break;
        case '6':
            run_module("player");
            break;
        case 'q':
        case 'Q':
            printf("\033[H\033[J"); // Clear terminal
            printf("Thanks for using HydrGn Modular System!\n");
            exit(0);
            break;
        default:
            // Invalid input, just continue
            break;
    }
}

// --- Main Application ---
int main() {
    // Save original terminal settings
    tcgetattr(STDIN_FILENO, &original_termios);
    
    // Set up main loop
    while (1) {
        print_main_menu();
        
        int ch = check_terminal_input();
        if (ch != -1) {
            process_input(ch);
        }
        
        usleep(100000); // 100ms delay to reduce CPU usage
    }

    return 0;
}