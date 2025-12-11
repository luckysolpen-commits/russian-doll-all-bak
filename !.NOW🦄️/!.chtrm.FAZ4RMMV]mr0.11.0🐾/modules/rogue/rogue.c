#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>

#define MAX_MAP_WIDTH 20
#define MAX_MAP_HEIGHT 20
#define MAX_INVENTORY_SLOTS 10
#define VAR_BUFFER_SIZE 4096

// Global variables for signal handling
int g_sig_received = 0;
int g_sig_num = 0;

// Function to ensure directories exist and save game state
void ensure_data_dir() {
    // Create data directory if it doesn't exist
    system("mkdir -p data");
    // Create debug directory if it doesn't exist
    system("mkdir -p '#.debug'");
}

// Function prototype declaration
bool load_game_state();
void update_needs();
void handle_death();

// Game and player data - LAYERED ARCHITECTURE
char original_map[MAX_MAP_HEIGHT][MAX_MAP_WIDTH];  // Layer 1: Static elements (walls, floors)
char placed_objects[MAX_MAP_HEIGHT][MAX_MAP_WIDTH];  // Layer 2: Items, etc. (not used in this version)
char player_layer[MAX_MAP_HEIGHT][MAX_MAP_WIDTH];    // Layer 3: Player position ('@' character)
int player_x = 10;
int player_y = 10;
int map_width = 20;
int map_height = 20;

// Game state variables
int player_hp = 100;
int max_hp = 100;
int turn_count = 0;
int hunger_level = 0;  // 0 = not hungry, 100 = starving
int thirst_level = 0;  // 0 = not thirsty, 100 = dying of thirst
int sleep_level = 0;   // 0 = well rested, 100 = desperate for sleep
int game_status = 0;   // 0 = playing, 1 = won, 2 = lost
char game_message[256] = "Welcome to the Roguelike! Use arrow keys to move.";
char inventory[MAX_INVENTORY_SLOTS][20];  // Simple inventory system
int inventory_count = 0;

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

// Initialize the game layers
void init_game() {
    // Initialize map layers
    for (int r = 0; r < map_height; r++) {
        for (int c = 0; c < map_width; c++) {
            if (r == 0 || r == map_height - 1 || c == 0 || c == map_width - 1) {
                original_map[r][c] = '#';  // Wall on borders
            } else if ((r >= 5 && r < 15) && (c >= 5 && c < 15)) {
                original_map[r][c] = '.';  // Floor in center room
            } else {
                original_map[r][c] = '#';  // Wall elsewhere
            }
            
            placed_objects[r][c] = ' ';    // Layer 2: Empty initially
            player_layer[r][c] = ' ';      // Layer 3: Empty initially
        }
    }
    
    // Place the player at the starting position in the player layer
    player_x = 10;
    player_y = 10;
    player_layer[player_y][player_x] = '@';  // Player is '@' in its own layer
    
    // Initialize game state
    player_hp = 100;
    turn_count = 0;
    hunger_level = 0;
    thirst_level = 0;
    sleep_level = 0;
    game_status = 0;
    strcpy(game_message, "Welcome to the Roguelike! Use arrow keys to move.");
    
    // Initialize inventory
    for (int i = 0; i < MAX_INVENTORY_SLOTS; i++) {
        strcpy(inventory[i], "");
    }
    inventory_count = 0;
    
    // Add debug log when init_game is called
    FILE* debug_file = fopen("#.debug/init_debug.txt", "a");
    if (debug_file != NULL) {
        fprintf(debug_file, "init_game() called at: ");
        time_t now = time(NULL);
        fprintf(debug_file, "%s", ctime(&now));
        fprintf(debug_file, "Position reset to: (%d, %d)\n\n", player_x, player_y);
        fclose(debug_file);
    }
}

// Update game display to stdout (for the chtm renderer to read)
void display_map() {
    // Compose the layers: original_map + placed_objects + player_layer
    char composite_map[MAX_MAP_HEIGHT][MAX_MAP_WIDTH];
    
    for (int r = 0; r < map_height; r++) {
        for (int c = 0; c < map_width; c++) {
            // Start with original map
            composite_map[r][c] = original_map[r][c];
            
            // Overlay placed objects (if not empty space)
            if (placed_objects[r][c] != ' ') {
                composite_map[r][c] = placed_objects[r][c];
            }
            
            // Overlay player on top of everything (highest priority)
            if (player_layer[r][c] != ' ') {
                composite_map[r][c] = player_layer[r][c];
            }
            
            printf("%c", composite_map[r][c]);
        }
        printf("\n");
    }
    
    // Reset buffer and add variables
    buf_pos = 0;
    ADD_VAR_INT(player_x, player_x);
    ADD_VAR_INT(player_y, player_y);
    ADD_VAR_INT(player_hp, player_hp);
    ADD_VAR_INT(max_hp, max_hp);
    ADD_VAR_INT(turn_count, turn_count);
    ADD_VAR_INT(hunger_level, hunger_level);
    ADD_VAR_INT(thirst_level, thirst_level);
    ADD_VAR_INT(sleep_level, sleep_level);
    ADD_VAR_INT(game_status, game_status);
    ADD_VAR_STR(game_message, game_message);
    
    // Output batched variables
    printf("VARS:%s;\n", var_buffer);
    
    // Update the map_from_module.txt file to reflect current composite map state
    FILE* map_file = fopen("#.debug/map_from_module.txt", "w");
    if (map_file != NULL) {
        for (int r = 0; r < map_height; r++) {
            for (int c = 0; c < map_width; c++) {
                fprintf(map_file, "%c ", composite_map[r][c]);
            }
            fprintf(map_file, "\n");
        }
        fclose(map_file);
    }
    
    // Add debug info about the display
    FILE* display_debug = fopen("#.debug/display_debug.txt", "a");
    if (display_debug != NULL) {
        fprintf(display_debug, "DISPLAY_CALLED_AT: ");
        time_t now = time(NULL);
        fprintf(display_debug, "%s", ctime(&now));
        fprintf(display_debug, "Position: (%d, %d)\n", player_x, player_y);
        fprintf(display_debug, "HP: %d, Hunger: %d, Thirst: %d, Sleep: %d\n", player_hp, hunger_level, thirst_level, sleep_level);
        fprintf(display_debug, "Map sent:\n");
        for (int r = 0; r < map_height; r++) {
            for (int c = 0; c < map_width; c++) {
                fprintf(display_debug, "%c", composite_map[r][c]);
            }
            fprintf(display_debug, "\n");
        }
        fprintf(display_debug, "VARS sent: %s;\n\n", var_buffer);
        fclose(display_debug);
    }
}

// Global frame counter for auditing
int frame_count = 0;

// Function to save game state to file
void save_game_state() {
    ensure_data_dir(); // Ensure data directory exists
    
    FILE* state_file = fopen("data/rogue_state.txt", "w");
    if (state_file != NULL) {
        // Save map dimensions
        fprintf(state_file, "WIDTH:%d\n", map_width);
        fprintf(state_file, "HEIGHT:%d\n", map_height);
        
        // Save player position
        fprintf(state_file, "PLAYER_X:%d\n", player_x);
        fprintf(state_file, "PLAYER_Y:%d\n", player_y);
        
        // Save game state
        fprintf(state_file, "PLAYER_HP:%d\n", player_hp);
        fprintf(state_file, "MAX_HP:%d\n", max_hp);
        fprintf(state_file, "TURN_COUNT:%d\n", turn_count);
        fprintf(state_file, "HUNGER_LEVEL:%d\n", hunger_level);
        fprintf(state_file, "THIRST_LEVEL:%d\n", thirst_level);
        fprintf(state_file, "SLEEP_LEVEL:%d\n", sleep_level);
        fprintf(state_file, "GAME_STATUS:%d\n", game_status);
        
        // Save the composite map state
        char composite_map[MAX_MAP_HEIGHT][MAX_MAP_WIDTH];
        
        for (int r = 0; r < map_height; r++) {
            for (int c = 0; c < map_width; c++) {
                // Start with original map
                composite_map[r][c] = original_map[r][c];
                
                // Overlay placed objects (if not empty space)
                if (placed_objects[r][c] != ' ') {
                    composite_map[r][c] = placed_objects[r][c];
                }
                
                // Overlay player on top of everything (highest priority)
                if (player_layer[r][c] != ' ') {
                    composite_map[r][c] = player_layer[r][c];
                }
                
                fprintf(state_file, "%c", composite_map[r][c]);
            }
            fprintf(state_file, "\n");
        }
        
        fclose(state_file);
        printf("Game state saved to data/rogue_state.txt\n");
    } else {
        printf("Error: Could not save game state to data/rogue_state.txt\n");
    }
}

// Move player based on command - updates player layer only
void move_player(char direction) {
    int new_x = player_x;
    int new_y = player_y;
    
    // Calculate new position based on direction
    switch (direction) {
        case 'U': // Up
            new_y--;
            break;
        case 'D': // Down
            new_y++;
            break;
        case 'L': // Left
            new_x--;
            break;
        case 'R': // Right
            new_x++;
            break;
    }
    
    // Check bounds
    if (new_x < 0 || new_x >= map_width || new_y < 0 || new_y >= map_height) {
        strcpy(game_message, "Cannot move there - out of bounds!");
        return;
    }
    
    // Check for walls
    if (original_map[new_y][new_x] == '#') {
        strcpy(game_message, "Cannot move there - wall blocking the way!");
        return;
    }
    
    // Clear the player from the current position in player layer
    player_layer[player_y][player_x] = ' ';
    
    // Update position
    player_x = new_x;
    player_y = new_y;
    
    // Place player at new position in player layer
    player_layer[player_y][player_x] = '@';
    
    strcpy(game_message, "You move carefully through the wasteland.");
    turn_count++;
    
    // Update frame counter and send current state variables in batch
    frame_count++;
    
    // Reset buffer and add variables
    buf_pos = 0;
    ADD_VAR_INT(frame_count, frame_count);
    ADD_VAR_INT(player_x, player_x);
    ADD_VAR_INT(player_y, player_y);
    ADD_VAR_INT(turn_count, turn_count);
    ADD_VAR_INT(player_hp, player_hp);
    ADD_VAR_INT(max_hp, max_hp);
    ADD_VAR_INT(hunger_level, hunger_level);
    ADD_VAR_INT(thirst_level, thirst_level);
    ADD_VAR_INT(sleep_level, sleep_level);
    ADD_VAR_INT(game_status, game_status);
    ADD_VAR_STR(game_message, game_message);
    
    // Output batched variables
    printf("VARS:%s;\n", var_buffer);
    
    // Update survival needs after movement
    update_needs();
    
    fflush(stdout);
}

// Update survival needs each turn
void update_needs() {
    // Increase needs each turn
    hunger_level += 1;
    thirst_level += 1;
    sleep_level += 1;
    
    // Cap at 100
    if (hunger_level > 100) hunger_level = 100;
    if (thirst_level > 100) thirst_level = 100;
    if (sleep_level > 100) sleep_level = 100;
    
    // Check for critical levels
    if (hunger_level >= 90) {
        strcpy(game_message, "You are starving! Find food soon!");
        player_hp -= 2; // Take damage when starving
    }
    if (thirst_level >= 90) {
        strcpy(game_message, "You are dying of thirst! Find water soon!");
        player_hp -= 2; // Take damage when dehydrated
    }
    if (sleep_level >= 90) {
        strcpy(game_message, "You desperately need sleep! Find a safe place to rest!");
        player_hp -= 1; // Take damage when exhausted
    }
    
    // Check for death
    if (player_hp <= 0) {
        handle_death();
    }
    
    // If player status changed (HP, needs, etc.), send update
    buf_pos = 0;
    ADD_VAR_INT(player_hp, player_hp);
    ADD_VAR_INT(hunger_level, hunger_level);
    ADD_VAR_INT(thirst_level, thirst_level);
    ADD_VAR_INT(sleep_level, sleep_level);
    ADD_VAR_INT(game_status, game_status);
    ADD_VAR_STR(game_message, game_message);
    printf("VARS:%s;\n", var_buffer);
    fflush(stdout);
}

// Handle player death
void handle_death() {
    game_status = 2; // Lost
    strcpy(game_message, "You have died! Press R to restart.");
}

// Signal handler to save state when process is terminated
void signal_handler(int sig) {
    // Only set flags and use async-signal-safe functions
    g_sig_received = 1;
    g_sig_num = sig;
    
    // Write directly to file descriptor (async-signal-safe)
    char msg[256];
    int fd = open("#.debug/quit.txt", O_WRONLY | O_CREAT | O_APPEND, 0644);
    
    if (fd != -1) {
        int len = snprintf(msg, sizeof(msg), "[Signal handler] Signal %d received\n", sig);
        write(fd, msg, len);
        close(fd);
    }
    
    // Don't do complex operations in signal handler - just set flag
    // Actual saving will happen in main loop
}

// Function to load game state from file
bool load_game_state() {
    FILE* state_file = fopen("data/rogue_state.txt", "r");
    if (state_file == NULL) {
        // No state file exists, return false
        return false;
    }
    
    char line[512]; // Increased buffer size to handle wide maps
    int width = -1, height = -1;
    int player_x_loaded = -1, player_y_loaded = -1;
    int player_hp_loaded = -1, max_hp_loaded = -1, turn_count_loaded = -1;
    int hunger_level_loaded = -1, thirst_level_loaded = -1, sleep_level_loaded = -1;
    int game_status_loaded = -1;
    int map_data_start_line = -1;
    int current_line = 0;
    
    // First pass: Read metadata and determine where map data begins
    while (fgets(line, sizeof(line), state_file)) {
        current_line++;
        // Remove newline character
        line[strcspn(line, "\n")] = 0;
        
        // Parse metadata lines
        if (strncmp(line, "WIDTH:", 6) == 0) {
            width = atoi(line + 6);
        } else if (strncmp(line, "HEIGHT:", 7) == 0) {
            height = atoi(line + 7);
        } else if (strncmp(line, "PLAYER_X:", 9) == 0) {
            player_x_loaded = atoi(line + 9);
        } else if (strncmp(line, "PLAYER_Y:", 9) == 0) {
            player_y_loaded = atoi(line + 9);
        } else if (strncmp(line, "PLAYER_HP:", 10) == 0) {
            player_hp_loaded = atoi(line + 10);
        } else if (strncmp(line, "MAX_HP:", 7) == 0) {
            max_hp_loaded = atoi(line + 7);
        } else if (strncmp(line, "TURN_COUNT:", 11) == 0) {
            turn_count_loaded = atoi(line + 11);
        } else if (strncmp(line, "HUNGER_LEVEL:", 13) == 0) {
            hunger_level_loaded = atoi(line + 13);
        } else if (strncmp(line, "THIRST_LEVEL:", 13) == 0) {
            thirst_level_loaded = atoi(line + 13);
        } else if (strncmp(line, "SLEEP_LEVEL:", 12) == 0) {
            sleep_level_loaded = atoi(line + 12);
        } else if (strncmp(line, "GAME_STATUS:", 12) == 0) {
            game_status_loaded = atoi(line + 12);
        } else {
            // First non-metadata line is the start of map data
            map_data_start_line = current_line;
            break;
        }
    }
    fclose(state_file);
    
    // Check if we have all required metadata
    if (width <= 0 || height <= 0 || width > MAX_MAP_WIDTH || height > MAX_MAP_HEIGHT ||
        player_x_loaded < 0 || player_y_loaded < 0 || player_x_loaded >= width || player_y_loaded >= height ||
        player_hp_loaded < 0 || max_hp_loaded < 0 || turn_count_loaded < 0 ||
        hunger_level_loaded < 0 || thirst_level_loaded < 0 || sleep_level_loaded < 0 ||
        game_status_loaded < 0 || game_status_loaded > 2) {
        return false;
    }
    
    // Now load the full file again to get map data
    state_file = fopen("data/rogue_state.txt", "r");
    if (state_file == NULL) {
        return false;
    }
    
    // Skip metadata lines by reading them again
    for (int i = 0; i < map_data_start_line - 1; i++) {
        if (!fgets(line, sizeof(line), state_file)) {
            fclose(state_file);
            return false;
        }
    }
    
    // Initialize map with default values
    map_width = width;
    map_height = height;
    for (int r = 0; r < MAX_MAP_HEIGHT; r++) {
        for (int c = 0; c < MAX_MAP_WIDTH; c++) {
            if (r < height && c < width) {
                original_map[r][c] = '.';      // Default background
                placed_objects[r][c] = ' ';    // No placed objects initially
                player_layer[r][c] = ' ';      // No player initially
            } else {
                original_map[r][c] = '.';      // Default outside bounds
                placed_objects[r][c] = ' ';    // Default outside bounds
                player_layer[r][c] = ' ';      // Default outside bounds
            }
        }
    }
    
    // Load the map data lines character by character
    int row = 0;
    while (fgets(line, sizeof(line), state_file) && row < height) {
        // Remove newline character
        line[strcspn(line, "\n")] = 0;
        
        // Handle lines that may have space-separated characters (e.g., "p . # . !")
        int col = 0;
        for (int i = 0; i < strlen(line) && col < width; i++) {
            char ch = line[i];
            if (ch != ' ') {  // Only process non-space characters
                if (col < width) {
                    // For '@' character in the saved grid, treat as potential player position
                    // But only the one at PLAYER_X, PLAYER_Y coordinates becomes the active player
                    if (ch == '@') {
                        if (row == player_y_loaded && col == player_x_loaded) {
                            // This is the current player position - put in player layer only
                            player_layer[row][col] = '@';
                            original_map[row][col] = '.';  // Background 
                            placed_objects[row][col] = ' ';   // No placed object
                        } else {
                            // This is a '@' that was previously at this position, convert to background
                            // This handles the case where old player positions appear in the saved grid
                            original_map[row][col] = '.';  // Convert to background
                            placed_objects[row][col] = ' ';   // No placed object
                            player_layer[row][col] = ' ';     // No player here
                        }
                    } else if (ch != '.' && ch != '#') {
                        // This is a placed object (like items, etc.)
                        original_map[row][col] = ch;  // Background might be different
                        placed_objects[row][col] = ' ';    // No placed object here if it's just background
                        player_layer[row][col] = ' ';     // No player here
                    } else {
                        // This is a background character ('.', '#', or other)
                        original_map[row][col] = ch;
                        placed_objects[row][col] = ' ';   // No placed object
                        player_layer[row][col] = ' ';     // No player here
                    }
                    col++;
                }
            }
        }
        
        // Fill remaining columns with default background if any are missing
        for (int c = col; c < width; c++) {
            original_map[row][c] = '.';
            placed_objects[row][c] = ' ';
            player_layer[row][c] = ' ';
        }
        
        row++;
    }
    fclose(state_file);
    
    // Set player position and game state
    player_x = player_x_loaded;
    player_y = player_y_loaded;
    player_hp = player_hp_loaded;
    max_hp = max_hp_loaded;
    turn_count = turn_count_loaded;
    hunger_level = hunger_level_loaded;
    thirst_level = thirst_level_loaded;
    sleep_level = sleep_level_loaded;
    game_status = game_status_loaded;
    
    // Debug output to quit.txt
    FILE* debug_file = fopen("#.debug/quit.txt", "a");
    if (debug_file != NULL) {
        time_t current_time = time(NULL);
        fprintf(debug_file, "[%s] Loaded game state: %dx%d, player at (%d,%d), HP: %d, hunger: %d, thirst: %d, sleep: %d, game_status: %d\n", 
               ctime(&current_time), width, height, player_x_loaded, player_y_loaded, player_hp_loaded, 
               hunger_level_loaded, thirst_level_loaded, sleep_level_loaded, game_status_loaded);
        fclose(debug_file);
    }
    
    return true;
}

int main() {
    char input[256];
    
    // Debug log when module starts
    FILE* debug_file = fopen("#.debug/quit.txt", "a");
    if (debug_file != NULL) {
        time_t current_time = time(NULL);
        fprintf(debug_file, "[%s] Roguelike Module Started\n", ctime(&current_time));
        fclose(debug_file);
    }
    
    // Check for saved state and load if available
    bool state_loaded = load_game_state();
    
    if (state_loaded) {
        // State was loaded, no need to initialize with defaults
        debug_file = fopen("#.debug/quit.txt", "a");
        if (debug_file != NULL) {
            time_t current_time = time(NULL);
            fprintf(debug_file, "[%s] Game state loaded successfully from file\n", ctime(&current_time));
            fclose(debug_file);
        }
        
        // After loading state, also call display_map to send loaded data to IPC
        // This ensures that the loaded state is sent to the renderer immediately
        display_map();
    } else {
        // No saved state, initialize game with default size
        init_game();
        debug_file = fopen("#.debug/quit.txt", "a");
        if (debug_file != NULL) {
            time_t current_time = time(NULL);
            fprintf(debug_file, "[%s] No saved state found, initialized with defaults\n", ctime(&current_time));
            fclose(debug_file);
        }
    }
    
    // Register signal handlers to save state on termination
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGQUIT, signal_handler);
    signal(SIGHUP, signal_handler);
    signal(SIGUSR1, signal_handler);
    signal(SIGUSR2, signal_handler);
    
    // Register exit function to save state on normal exit
    atexit(save_game_state);
    
    // Main loop to process commands from parent process (CHTM renderer)
    while (fgets(input, sizeof(input), stdin) && !g_sig_received) {
        // Remove newline character
        input[strcspn(input, "\n")] = 0;
        
        // Check if a signal was received that requires termination
        if (g_sig_received) {
            break;
        }
        
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
            if (strlen(canvas_id) > 0 && strcmp(canvas_id, "game_map") == 0) {
                // Handle game map - send composite map
                display_map();
            } else {
                // Handle default game display - send composite map
                display_map();
            }
            fflush(stdout);
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
                    // Treat as game map movement
                    if (game_status == 0) {  // Only if game is still active
                        move_player(direction);
                    } else if (game_status == 2 && key_code == 1001) {  // If dead and right arrow pressed, restart
                        init_game();
                        strcpy(game_message, "Game restarted! Use arrow keys to move.");
                        buf_pos = 0;
                        ADD_VAR_INT(player_x, player_x);
                        ADD_VAR_INT(player_y, player_y);
                        ADD_VAR_INT(turn_count, turn_count);
                        ADD_VAR_INT(player_hp, player_hp);
                        ADD_VAR_INT(max_hp, max_hp);
                        ADD_VAR_INT(hunger_level, hunger_level);
                        ADD_VAR_INT(thirst_level, thirst_level);
                        ADD_VAR_INT(sleep_level, sleep_level);
                        ADD_VAR_INT(game_status, game_status);
                        ADD_VAR_STR(game_message, game_message);
                        printf("VARS:%s;\n", var_buffer);
                        fflush(stdout);
                    }
                }
            }
        }
        else if (strncmp(input, "CLI_INPUT:", 10) == 0) {
            // Handle CLI input commands: CLI_INPUT:text
            char *command_text = input + 10; // Skip "CLI_INPUT:"
            
            if (strcmp(command_text, "r") == 0 || strcmp(command_text, "restart") == 0) {
                init_game();
                strcpy(game_message, "Game restarted! Use arrow keys to move.");
                
                // Update any relevant variables in batch
                buf_pos = 0;
                ADD_VAR_INT(player_x, player_x);
                ADD_VAR_INT(player_y, player_y);
                ADD_VAR_INT(turn_count, turn_count);
                ADD_VAR_INT(player_hp, player_hp);
                ADD_VAR_INT(max_hp, max_hp);
                ADD_VAR_INT(hunger_level, hunger_level);
                ADD_VAR_INT(thirst_level, thirst_level);
                ADD_VAR_INT(sleep_level, sleep_level);
                ADD_VAR_INT(game_status, game_status);
                ADD_VAR_STR(game_message, game_message);
                
                // Output batched variables
                printf("VARS:%s;\n", var_buffer);
                fflush(stdout);
            } else {
                // For other commands, just acknowledge but don't change game state
                // Update current state to reflect that the command was received
                buf_pos = 0;
                ADD_VAR_INT(player_x, player_x);
                ADD_VAR_INT(player_y, player_y);
                ADD_VAR_INT(turn_count, turn_count);
                ADD_VAR_INT(player_hp, player_hp);
                ADD_VAR_INT(max_hp, max_hp);
                ADD_VAR_INT(hunger_level, hunger_level);
                ADD_VAR_INT(thirst_level, thirst_level);
                ADD_VAR_INT(sleep_level, sleep_level);
                ADD_VAR_INT(game_status, game_status);
                ADD_VAR_STR(game_message, game_message);
                
                // Output batched variables
                printf("VARS:%s;\n", var_buffer);
                fflush(stdout);
            }
        }
        else if (strcmp(input, "QUIT") == 0) {
            // Debug logging to file
            FILE* debug_file = fopen("#.debug/quit.txt", "a");
            if (debug_file != NULL) {
                time_t current_time = time(NULL);
                fprintf(debug_file, "[%s] Received QUIT command, saving game state...\n", ctime(&current_time));
                fclose(debug_file);
            }
            
            printf("Received QUIT command, saving game state...\n");
            save_game_state();  // Save state before quitting
            printf("Game state saved, exiting...\n");
            
            // Additional debug logging after save
            debug_file = fopen("#.debug/quit.txt", "a");
            if (debug_file != NULL) {
                time_t current_time = time(NULL);
                fprintf(debug_file, "[%s] Game state saved, exiting...\n", ctime(&current_time));
                fclose(debug_file);
            }
            
            break;
        }
        
        fflush(stdout);
    }
    
    // Check if exit was due to signal
    if (g_sig_received) {
        FILE* debug_file = fopen("#.debug/quit.txt", "a");
        if (debug_file != NULL) {
            time_t current_time = time(NULL);
            fprintf(debug_file, "[%s] Signal %d received, saving game state...\n", ctime(&current_time), g_sig_num);
            fclose(debug_file);
        }
        
        save_game_state();
        
        debug_file = fopen("#.debug/quit.txt", "a");
        if (debug_file != NULL) {
            time_t current_time = time(NULL);
            fprintf(debug_file, "[%s] Game state saved from signal %d, exiting...\n", ctime(&current_time), g_sig_num);
            fclose(debug_file);
        }
    } else {
        // Save state before exiting (in case stdin was closed by parent)
        FILE* debug_file = fopen("#.debug/quit.txt", "a");
        if (debug_file != NULL) {
            time_t current_time = time(NULL);
            fprintf(debug_file, "[%s] Main loop exited normally, saving game state...\n", ctime(&current_time));
            fclose(debug_file);
        }
        
        save_game_state();
        
        debug_file = fopen("#.debug/quit.txt", "a");
        if (debug_file != NULL) {
            time_t current_time = time(NULL);
            fprintf(debug_file, "[%s] Game state saved from main exit, program ending...\n", ctime(&current_time));
            fclose(debug_file);
        }
    }
    
    return 0;
}