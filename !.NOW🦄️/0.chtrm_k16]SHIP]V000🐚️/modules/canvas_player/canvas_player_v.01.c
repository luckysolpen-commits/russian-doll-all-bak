#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#define MAX_CANVAS_WIDTH 20
#define MAX_CANVAS_HEIGHT 20
#define PALETTE_SIZE 8
#define VAR_BUFFER_SIZE 4096

// Canvas and player data - LAYERED ARCHITECTURE
char original_canvas[MAX_CANVAS_HEIGHT][MAX_CANVAS_WIDTH];  // Layer 1: Static elements (base layer)
char placed_objects[MAX_CANVAS_HEIGHT][MAX_CANVAS_WIDTH];  // Layer 2: Placed objects (symbols from palette)
char player_layer[MAX_CANVAS_HEIGHT][MAX_CANVAS_WIDTH];    // Layer 3: Player position ('p' character)
int player_x = 0;
int player_y = 0;
int canvas_width = 8;
int canvas_height = 8;

// Emoji palette data
char palette_symbols[PALETTE_SIZE] = {' ', '!', '#', '$', '%', '^', '&', '*'};
int selected_palette_index = 0; // Currently selected palette index
char palette_canvas[1][PALETTE_SIZE]; // Canvas for the emoji palette

// Buffer for batching variable updates
char var_buffer[VAR_BUFFER_SIZE];
int buf_pos = 0;

// Macro for adding variables to the buffer
#define ADD_VAR_INT(name, value) \
    buf_pos += snprintf(var_buffer + buf_pos, sizeof(var_buffer) - buf_pos, \
    "%s=%d,", #name, value)

// Specialized macro for character values
#define ADD_VAR_CHAR(name, value) \
    buf_pos += snprintf(var_buffer + buf_pos, sizeof(var_buffer) - buf_pos, \
    "%s=%c,", #name, value)

// Specialized macro for string values
#define ADD_VAR_STR(name, value) \
    buf_pos += snprintf(var_buffer + buf_pos, sizeof(var_buffer) - buf_pos, \
    "%s=%s,", #name, value)

// Generic macro that defaults to integer
#define ADD_VAR(name, value) ADD_VAR_INT(name, value)

// Initialize the canvas layers
void init_canvas() {
    for (int r = 0; r < canvas_height; r++) {
        for (int c = 0; c < canvas_width; c++) {
            original_canvas[r][c] = '.';      // Layer 1: Fill with placeholder
            placed_objects[r][c] = ' ';       // Layer 2: Empty initially
            player_layer[r][c] = ' ';         // Layer 3: Empty initially
        }
    }
    // Place the player at the starting position in the player layer
    if (canvas_height > 0 && canvas_width > 0) {
        player_layer[0][0] = 'p';  // Player is 'p' in its own layer
    }
    player_x = 0;
    player_y = 0;
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
    // Compose the layers: original_canvas + placed_objects + player_layer
    char composite_canvas[MAX_CANVAS_HEIGHT][MAX_CANVAS_WIDTH];
    
    for (int r = 0; r < canvas_height; r++) {
        for (int c = 0; c < canvas_width; c++) {
            // Start with original canvas
            composite_canvas[r][c] = original_canvas[r][c];
            
            // Overlay placed objects (if not empty space)
            if (placed_objects[r][c] != ' ') {
                composite_canvas[r][c] = placed_objects[r][c];
            }
            
            // Overlay player on top of everything (highest priority)
            if (player_layer[r][c] != ' ') {
                composite_canvas[r][c] = player_layer[r][c];
            }
            
            printf("%c ", composite_canvas[r][c]);
        }
        printf("\n");
    }
    
    // Reset buffer and add variables
    buf_pos = 0;
    ADD_VAR_INT(player_x, player_x);
    ADD_VAR_INT(player_y, player_y);
    
    // Output batched variables
    printf("VARS:%s;\n", var_buffer);
    
    // Update the map.txt file to reflect current composite canvas state
    FILE* map_file = fopen("map_from_module.txt", "w");
    if (map_file != NULL) {
        for (int r = 0; r < canvas_height; r++) {
            for (int c = 0; c < canvas_width; c++) {
                fprintf(map_file, "%c ", composite_canvas[r][c]);
            }
            fprintf(map_file, "\n");
        }
        fclose(map_file);
    }
}

// Update palette display to stdout - only sends data, no direct printing
void display_palette() {
    // Reset buffer and add variables
    buf_pos = 0;
    ADD_VAR(selected_palette_index, selected_palette_index);
    ADD_VAR_CHAR(selected_symbol, palette_symbols[selected_palette_index]);
    
    // Output batched variables
    printf("VARS:%s;\n", var_buffer);
    
    // Also send the actual palette line content so renderer can store it in model
    for (int c = 0; c < PALETTE_SIZE; c++) {
        putchar(palette_canvas[0][c]);
        if (c == selected_palette_index) {
            // Always use navigation indicator (">") for the selected palette item
            putchar('>');
        } else {
            // Add space after unselected symbols to maintain spacing
            putchar(' ');
        }
        if (c < PALETTE_SIZE - 1) {
            // Add consistent spacing between elements
            putchar(' ');
        }
    }
    putchar('\n');
}

// Global frame counter for auditing
int frame_count = 0;

// Move player based on command - updates player layer only
void move_player(char direction) {
    // Clear the player from the current position in player layer
    player_layer[player_y][player_x] = ' ';
    
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
    
    // Place player at new position in player layer
    player_layer[player_y][player_x] = 'p';
    
    // Update frame counter and send current state variables in batch
    frame_count++;
    
    // Reset buffer and add variables
    buf_pos = 0;
    ADD_VAR_INT(frame_count, frame_count);
    ADD_VAR_INT(player_x, player_x);
    ADD_VAR_INT(player_y, player_y);
    
    // Output batched variables
    printf("VARS:%s;\n", var_buffer);
    
    fflush(stdout);
}

// Place the selected symbol at the current player position - updates placed_objects layer
void place_selected_symbol() {
    char selected_symbol = palette_symbols[selected_palette_index];
    
    // Place the selected symbol in the placed_objects layer at player position
    placed_objects[player_y][player_x] = selected_symbol;
    
    // Send debug variables back to the CHTM framework in batch
    frame_count++;
    
    // Reset buffer and add variables
    buf_pos = 0;
    ADD_VAR_INT(frame_count, frame_count);
    ADD_VAR_STR(last_action, "placed");
    ADD_VAR_CHAR(last_placed_symbol, selected_symbol);
    ADD_VAR_INT(last_placed_at_x, player_x);
    ADD_VAR_INT(last_placed_at_y, player_y);
    ADD_VAR_CHAR(selected_symbol, selected_symbol);
    ADD_VAR_INT(selected_palette_index, selected_palette_index);
    ADD_VAR_INT(player_x, player_x);
    ADD_VAR_INT(player_y, player_y);
    
    // Output batched variables
    printf("VARS:%s;\n", var_buffer);
    
    fflush(stdout);
}

// NEW: Place an 'X' symbol at current player position - for testing
void place_x_symbol() {
    // Place an 'X' in the placed_objects layer at player position
    placed_objects[player_y][player_x] = 'X';
    
    // Send debug variables back to the CHTM framework in batch
    frame_count++;
    
    // Reset buffer and add variables
    buf_pos = 0;
    ADD_VAR_INT(frame_count, frame_count);
    ADD_VAR_STR(last_action, "placed_x");
    ADD_VAR_CHAR(last_placed_symbol, 'X');
    ADD_VAR_INT(last_placed_at_x, player_x);
    ADD_VAR_INT(last_placed_at_y, player_y);
    ADD_VAR_INT(player_x, player_x);
    ADD_VAR_INT(player_y, player_y);
    
    // Output batched variables
    printf("VARS:%s;\n", var_buffer);
    
    fflush(stdout);
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
    
    // Send updated selected symbol information after palette selection change in batch
    
    // Reset buffer and add variables
    buf_pos = 0;
    ADD_VAR_CHAR(selected_symbol, palette_symbols[selected_palette_index]);
    ADD_VAR(selected_palette_index, selected_palette_index);
    
    // Output batched variables
    printf("VARS:%s;\n", var_buffer);
    fflush(stdout);
}

int main() {
    char input[256];
    
    // Initialize canvas and palette with default size
    init_canvas();
    init_palette();
    
    printf("Canvas initialized with size %dx%d. Player at (0,0)\n", canvas_width, canvas_height);
    // Send initial position variables in batch
    
    // Reset buffer and add initial variables
    buf_pos = 0;
    ADD_VAR_INT(player_x, player_x);
    ADD_VAR_INT(player_y, player_y);
    ADD_VAR_INT(selected_palette_index, selected_palette_index);
    ADD_VAR_CHAR(selected_symbol, palette_symbols[selected_palette_index]);
    
    // Output batched variables
    printf("VARS:%s;\n", var_buffer);
    fflush(stdout);
    
    // Main loop to process commands from parent process (CHTM renderer)
    while (fgets(input, sizeof(input), stdin)) {
        // Remove newline character
        input[strcspn(input, "\n")] = 0;
        
        if (strncmp(input, "DISPLAY", 7) == 0) {  // Check if command starts with "DISPLAY"
            // Parse for canvas ID if present in format "DISPLAY:ELEMENT_ID:canvas_id"
            char canvas_id[256] = {0};
            char *element_id_part = strstr(input, ":ELEMENT_ID:");
            
            if (element_id_part != NULL) {
                // Extract the canvas ID after :ELEMENT_ID:
                char *id_start = element_id_part + 12; // Length of ":ELEMENT_ID:"
                strncpy(canvas_id, id_start, sizeof(canvas_id) - 1);
                canvas_id[sizeof(canvas_id) - 1] = '\0';
            }
            
            // Handle based on canvas ID
            if (strlen(canvas_id) > 0 && strcmp(canvas_id, "emoji_palette") == 0) {
                // Handle emoji palette - send data to be stored in model by renderer
                for (int c = 0; c < PALETTE_SIZE; c++) {
                    putchar(palette_canvas[0][c]);
                    if (c == selected_palette_index) {
                        // Always use navigation indicator (">") for the selected palette item
                        putchar('>');
                    } else {
                        // Add space after unselected symbols to maintain spacing
                        putchar(' ');
                    }
                    if (c < PALETTE_SIZE - 1) {
                        // Add consistent spacing between elements
                        putchar(' ');
                    }
                }
                putchar('\n');
                
                // Reset buffer and add variables
                buf_pos = 0;
                ADD_VAR_INT(selected_palette_index, selected_palette_index);
                ADD_VAR_CHAR(selected_symbol, palette_symbols[selected_palette_index]);
                
                // Output batched variables
                printf("VARS:%s;\n", var_buffer);
            } else {
                // Handle regular game canvas - send composite canvas
                display_canvas();
            }
            fflush(stdout);
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
        else if (strncmp(input, "KEY:", 4) == 0) {
            // Parse key code command: KEY:n:ELEMENT_ID:id where n is the ASCII code
            char *key_part = input + 4;
            char *element_id_part = strstr(key_part, ":ELEMENT_ID:");
            
            int key_code;
            char element_id[256] = {0};
            
            if (element_id_part != NULL) {
                // Extract the key code before :ELEMENT_ID:
                *element_id_part = '\0'; // Temporarily terminate the key part
                key_code = atoi(key_part);
                
                // Extract the element ID after :ELEMENT_ID:
                char *id_start = element_id_part + 12; // Length of ":ELEMENT_ID:"
                strncpy(element_id, id_start, sizeof(element_id) - 1);
                element_id[sizeof(element_id) - 1] = '\0';
                
                // Restore the original input string
                *element_id_part = ':';
            } else {
                // No element ID provided, just parse the key code
                key_code = atoi(key_part);
            }
            
            // Map raw ASCII codes to actions based on element ID
            if (key_code >= 1000) { // Arrow key codes (ARROW_LEFT = 1000, etc.)
                char direction;
                switch (key_code) {
                    case 1000: // ARROW_LEFT (defined as 1000 in main renderer)
                        direction = 'L';
                        break;
                    case 1001: // ARROW_RIGHT (defined as 1001 in main renderer)
                        direction = 'R';
                        break;
                    case 1002: // ARROW_UP (defined as 1002 in main renderer)
                        direction = 'U';
                        break;
                    case 1003: // ARROW_DOWN (defined as 1003 in main renderer)
                        direction = 'D';
                        break;
                    default:
                        direction = 0; // No valid direction
                        break;
                }
                
                if (direction != 0) {
                    // If element ID is "emoji_palette", handle as palette navigation
                    if (strlen(element_id) > 0 && strcmp(element_id, "emoji_palette") == 0) {
                        move_palette_selection(direction);
                    } else {
                        // Otherwise, treat as game canvas movement
                        move_player(direction);
                    }
                }
            }
            else if (key_code == 88 || key_code == 120) { // 'X' key (uppercase or lowercase)
                // Handle X key to place an 'X' symbol at current player position
                place_x_symbol();
            }
            else if (key_code == 32) { // Spacebar
                // Handle spacebar key press with element targeting
                if (strlen(element_id) > 0 && strcmp(element_id, "emoji_palette") == 0) {
                    // On emoji palette, spacebar could trigger selection
                    // Reset buffer and add variables
                    buf_pos = 0;
                    ADD_VAR_STR(space_pressed_on, "palette");
                    ADD_VAR_CHAR(selected_symbol, palette_symbols[selected_palette_index]);
                    
                    // Output batched variables
                    printf("VARS:%s;\n", var_buffer);
                } else {
                    // On other canvases (like game canvas), spacebar could trigger action
                    // Reset buffer and add variables
                    buf_pos = 0;
                    ADD_VAR_STR(space_pressed_on, "canvas");
                    ADD_VAR_INT(space_pressed_at_x, player_x);
                    ADD_VAR_INT(space_pressed_at_y, player_y);
                    
                    // Output batched variables
                    printf("VARS:%s;\n", var_buffer);
                }
                fflush(stdout);
            }
            else if (key_code == 10) { // Enter key
                // Handle Enter key press with element targeting
                if (strlen(element_id) > 0 && strcmp(element_id, "emoji_palette") == 0) {
                    // On emoji palette, Enter could confirm selection
                    // Reset buffer and add variables
                    buf_pos = 0;
                    ADD_VAR_STR(enter_pressed_on, "palette");
                    ADD_VAR_CHAR(confirmed_symbol, palette_symbols[selected_palette_index]);
                    
                    // Output batched variables
                    printf("VARS:%s;\n", var_buffer);
                    // Place the selected symbol at player position
                 //   place_selected_symbol();
                } else {
                    // On other canvases (like game canvas), Enter could trigger action
                    // Reset buffer and add variables
                    buf_pos = 0;
                    ADD_VAR_STR(enter_pressed_on, "canvas");
                    ADD_VAR_STR(enter_action_triggered, "yes");
                    
                    // Output batched variables
                    printf("VARS:%s;\n", var_buffer);
                    // For now just place the selected symbol
                //    place_selected_symbol();
                }
                fflush(stdout);
            }
            else if (key_code >= '0' && key_code <= '9') { // Number keys for palette selection
                // Handle palette selection via number keys
                int index = key_code - '0';
                if (index < PALETTE_SIZE) {
                    selected_palette_index = index;
                    // Send updated selected symbol information after palette selection change in batch
                    
                    // Reset buffer and add variables
                    buf_pos = 0;
                    ADD_VAR_CHAR(selected_symbol, palette_symbols[selected_palette_index]);
                    ADD_VAR_INT(selected_palette_index, selected_palette_index);
                    
                    // Output batched variables
                    printf("VARS:%s;\n", var_buffer);
                    fflush(stdout);
                }
            }
            else {
                // Ignore other raw key codes for now
            }
        }
        // Keep legacy MOVE: and PALETTE_MOVE: commands for compatibility during transition
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
            // Return player position as variables in batch
            
            // Reset buffer and add variables
            buf_pos = 0;
            ADD_VAR_INT(player_x, player_x);
            ADD_VAR_INT(player_y, player_y);
            
            // Output batched variables
            printf("VARS:%s;\n", var_buffer);
        }
        else if (strcmp(input, "GET_PALETTE") == 0) {
            // Return palette information as variables in batch
            
            // Reset buffer and add variables
            buf_pos = 0;
            ADD_VAR_INT(selected_palette_index, selected_palette_index);
            ADD_VAR_CHAR(selected_symbol, palette_symbols[selected_palette_index]);
            
            // Output batched variables
            printf("VARS:%s;\n", var_buffer);
        }
        else if (strcmp(input, "PLACE") == 0) {
            // Place the selected symbol at the current player position
          //  place_selected_symbol();
        }
        // Spacebar ("32") command handling has been removed for sanitary testing
        // else if (strcmp(input, "32") == 0) {  // Spacebar
        //     // Handle spacebar key press (ASCII 32) - only send variables, don't update canvas state
        //     printf("VAR:last_input_was_space=yes\n");
        //     printf("VAR:space_pressed_at_x=%d\n", player_x);
        //     printf("VAR:space_pressed_at_y=%d\n", player_y);
        //     printf("VAR:last_action=placed\n");
        //     printf("VAR:last_placed_symbol=%c\n", palette_symbols[selected_palette_index]);
        //     printf("VAR:last_placed_at_x=%d\n", player_x);
        //     printf("VAR:last_placed_at_y=%d\n", player_y);
        //     printf("VAR:selected_symbol=%c\n", palette_symbols[selected_palette_index]);
        //     printf("VAR:selected_palette_index=%d\n", selected_palette_index);
        //     fflush(stdout);
        //     // Do NOT call place_selected_symbol() to avoid canvas state changes
        // }
        else if (strncmp(input, "CLI_INPUT:", 10) == 0) {
            // Handle CLI input commands: CLI_INPUT:text
            char *command_text = input + 10; // Skip "CLI_INPUT:"
            printf("Received CLI command: %s\n", command_text);
            
            // Process the command based on the input
            if (strcmp(command_text, "help") == 0) {
                printf("Available commands: help, clear, quit\n");
            } else if (strcmp(command_text, "clear") == 0) {
                // Clear the canvas grid back to default
                init_canvas();
                printf("Canvas cleared.\n");
            } else if (strcmp(command_text, "quit") == 0) {
                printf("Quitting...\n");
            } else {
                // For unrecognized commands, just acknowledge
                printf("Command '%s' received but not recognized.\n", command_text);
            }
            
            // Update any relevant variables in batch
            
            // Reset buffer and add variables
            buf_pos = 0;
            ADD_VAR_STR(last_cli_command, command_text);
            ADD_VAR_STR(command_input, command_text);
            
            // Output batched variables
            printf("VARS:%s;\n", var_buffer);
            fflush(stdout);
        }
        else if (strcmp(input, "QUIT") == 0) {
            break;
        }
        
        fflush(stdout);
    }
    
    return 0;
}
