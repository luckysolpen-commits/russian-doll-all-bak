#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#define MAX_CANVAS_WIDTH 20
#define MAX_CANVAS_HEIGHT 20
#define PALETTE_SIZE 8

// Canvas and player data
char canvas[MAX_CANVAS_HEIGHT][MAX_CANVAS_WIDTH];
int player_x = 0;
int player_y = 0;
int canvas_width = 8;
int canvas_height = 8;

// Emoji palette data
char palette_symbols[PALETTE_SIZE] = {' ', '!', '#', '$', '%', '^', '&', '*'};
int selected_palette_index = 0; // Currently selected palette index
char palette_canvas[1][PALETTE_SIZE]; // Canvas for the emoji palette

// Initialize the canvas
void init_canvas() {
    for (int r = 0; r < canvas_height; r++) {
        for (int c = 0; c < canvas_width; c++) {
            canvas[r][c] = '.';  // Fill with placeholder
        }
    }
    // Place the player at the starting position
    if (canvas_height > 0 && canvas_width > 0) {
        canvas[0][0] = 'p';  // Player is 'p'
    }
}

// Initialize the palette canvas
void init_palette() {
    for (int c = 0; c < PALETTE_SIZE; c++) {
        palette_canvas[0][c] = palette_symbols[c];
    }
    // Set initial selection to the first element (blank)
    selected_palette_index = 0;
}

// Update canvas display to stdout (for the chtm renderer to read)
void display_canvas() {
    for (int r = 0; r < canvas_height; r++) {
        for (int c = 0; c < canvas_width; c++) {
            printf("%c ", canvas[r][c]);
        }
        printf("\n");
    }
    // Also send player position as variables for the renderer
    printf("VAR:player_x=%d\n", player_x);
    printf("VAR:player_y=%d\n", player_y);
}

// Update palette display to stdout
void display_palette() {
    for (int c = 0; c < PALETTE_SIZE; c++) {
        printf("%c", palette_canvas[0][c]);
        if (c == selected_palette_index) {
            // Always use navigation indicator (">") for the selected palette item
            printf(">");
        } else {
            // Add space after unselected symbols to maintain spacing
            printf(" ");
        }
        if (c < PALETTE_SIZE - 1) {
            // Add consistent spacing between elements
            printf(" ");
        }
    }
    printf("\n");
    // Send the selected palette index as a variable
    printf("VAR:selected_palette_index=%d\n", selected_palette_index);
    printf("VAR:selected_symbol=%c\n", palette_symbols[selected_palette_index]);
}

// Move player based on command
void move_player(char direction) {
    // Clear current player position
    canvas[player_y][player_x] = '.';
    
    // Update position based on direction
    switch (direction) {
        case 'U': // Up
            if (player_y > 0) player_y--;
            break;
        case 'D': // Down
            if (player_y < canvas_height - 1) player_y++;
            break;
        case 'L': // Left
            if (player_x > 0) player_x--;
            break;
        case 'R': // Right
            if (player_x < canvas_width - 1) player_x++;
            break;
    }
    
    // Place player at new position
    canvas[player_y][player_x] = 'p';
}

// Move selection in palette based on command
void move_palette_selection(char direction) {
    switch (direction) {
        case 'L': // Left - move to previous item
            if (selected_palette_index > 0) selected_palette_index--;
            break;
        case 'R': // Right - move to next item
            if (selected_palette_index < PALETTE_SIZE - 1) selected_palette_index++;
            break;
        case '0': // Select index 0 (blank)
            selected_palette_index = 0;
            break;
        case '1': // Select index 1
            if (selected_palette_index < PALETTE_SIZE - 1) selected_palette_index = 1;
            break;
        case '2': // Select index 2
            if (selected_palette_index < PALETTE_SIZE - 1) selected_palette_index = 2;
            break;
        case '3': // Select index 3
            if (selected_palette_index < PALETTE_SIZE - 1) selected_palette_index = 3;
            break;
        case '4': // Select index 4
            if (selected_palette_index < PALETTE_SIZE - 1) selected_palette_index = 4;
            break;
        case '5': // Select index 5
            if (selected_palette_index < PALETTE_SIZE - 1) selected_palette_index = 5;
            break;
        case '6': // Select index 6
            if (selected_palette_index < PALETTE_SIZE - 1) selected_palette_index = 6;
            break;
        case '7': // Select index 7
            if (selected_palette_index < PALETTE_SIZE - 1) selected_palette_index = 7;
            break;
    }
}

int main() {
    char input[256];
    
    // Initialize canvas and palette with default size
    init_canvas();
    init_palette();
    
    printf("Canvas initialized with size %dx%d. Player at (0,0)\n", canvas_width, canvas_height);
    // Send initial position variables
    printf("VAR:player_x=%d\n", player_x);
    printf("VAR:player_y=%d\n", player_y);
    printf("VAR:selected_palette_index=%d\n", selected_palette_index);
    printf("VAR:selected_symbol=%c\n", palette_symbols[selected_palette_index]);
    fflush(stdout);
    
    // Main loop to process commands from parent process (CHTM renderer)
    while (fgets(input, sizeof(input), stdin)) {
        // Remove newline character
        input[strcspn(input, "\n")] = 0;
        
        if (strcmp(input, "DISPLAY") == 0) {
            // Only send canvas and variables, not initialization messages
            display_canvas();
        }
        else if (strcmp(input, "DISPLAY_PALETTE") == 0) {
            // Display the emoji palette (always uses navigation indicator)
            display_palette();
        }
        else if (strncmp(input, "SIZE:", 5) == 0) {
            // Parse size command: SIZE:width,height
            char *size_part = input + 5;
            char *comma = strchr(size_part, ',');
            if (comma) {
                *comma = '\0';
                int new_width = atoi(size_part);
                int new_height = atoi(comma + 1);
                
                if (new_width > 0 && new_width <= MAX_CANVAS_WIDTH && 
                    new_height > 0 && new_height <= MAX_CANVAS_HEIGHT) {
                    canvas_width = new_width;
                    canvas_height = new_height;
                    init_canvas();  // Reinitialize with new size
                    printf("Canvas resized to %dx%d\n", canvas_width, canvas_height);
                }
            }
        }
        else if (strncmp(input, "PALETTE_SIZE:", 13) == 0) {
            // Parse palette size command: PALETTE_SIZE:width,height
            char *size_part = input + 13;
            char *comma = strchr(size_part, ',');
            if (comma) {
                *comma = '\0';
                int new_width = atoi(size_part);
                int new_height = atoi(comma + 1);
                
                if (new_width == 8 && new_height == 1) { // Expected palette size
                    // Palette dimensions are as expected, no action needed
                    printf("Palette initialized with size %dx%d\n", new_width, new_height);
                }
            }
        }
        else if (strncmp(input, "MOVE:", 5) == 0) {
            // Parse move command: MOVE:U, MOVE:D, MOVE:L, MOVE:R
            char direction = input[5];
            move_player(direction);
            // Don't print position update here, DISPLAY command will handle that
        }
        else if (strncmp(input, "PALETTE_MOVE:", 13) == 0) {
            // Parse palette move command: PALETTE_MOVE:L, PALETTE_MOVE:R, PALETTE_MOVE:0-7
            char *direction = input + 13;
            move_palette_selection(*direction);
        }
        else if (strcmp(input, "GET_POS") == 0) {
            // Return player position as variables
            printf("VAR:player_x=%d\n", player_x);
            printf("VAR:player_y=%d\n", player_y);
        }
        else if (strcmp(input, "GET_PALETTE") == 0) {
            // Return palette information as variables
            printf("VAR:selected_palette_index=%d\n", selected_palette_index);
            printf("VAR:selected_symbol=%c\n", palette_symbols[selected_palette_index]);
        }
        else if (strcmp(input, "QUIT") == 0) {
            break;
        }
        
        fflush(stdout);
    }
    
    return 0;
}