#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // for usleep
#include <time.h>

#define MAX_FILENAME 256
#define MAX_MAP_SIZE 100 // Arbitrary limit for map dimensions

// Global variables
FILE *file = NULL;
FILE *fp2 = NULL;
int rows = 0, cols = 0, cols_rows = 0;
int c;
int **map_matrix = NULL;
int player_x = 5;  // Default x-coordinate
int player_y = 5;  // Default y-coordinate
int score = 0;
long last_history_position = 0; // Track position in history file

// Constants for map symbols
#define PLAYER_SYMBOL '@'  // '@' for player
#define SPACE_SYMBOL '.'   // '.' for empty space
#define WALL_SYMBOL '#'    // '#' for walls
#define TREASURE_SYMBOL 'T' // 'T' for treasure
#define ROCK_SYMBOL 'R'     // 'R' for rocks

// Function prototypes
void load_map(const char *map_name);
void render_print();
void update_map_0(int x_coords, int y_coords);
void process_key_command(int key);
void free_map_matrix();
void update_player_stats();

// Load map from file - SIMPLE approach
void load_map(const char *map_name) {
    char filename[MAX_FILENAME];
    snprintf(filename, MAX_FILENAME, "pieces/world/%s", map_name);

    // Free previous map matrix if it exists
    if (map_matrix != NULL) {
        free_map_matrix();
    }

    // Open the map file
    file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Error opening map file: %s\n", filename);
        // Create a default map if file doesn't exist
        file = fopen(filename, "w");
        if (file) {
            // Write a default map
            fprintf(file, "####################\n");
            fprintf(file, "#  ...............T#\n"); 
            fprintf(file, "#  ...............T#\n");
            fprintf(file, "#  ....R@.........T#\n");
            fprintf(file, "#  ....R..........T#\n");
            fprintf(file, "#  ....R..........T#\n");
            fprintf(file, "#  ....R..........T#\n");
            fprintf(file, "#  ................#\n");
            fprintf(file, "#                  #\n");
            fprintf(file, "####################\n");
            fclose(file);
            file = fopen(filename, "r");
        }
        if (!file) {
            fprintf(stderr, "Could not create default map file: %s\n", filename);
            exit(1);
        }
    }

    // First pass: count rows and find longest row
    rows = 0;
    cols_rows = 0;
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        int line_len = strlen(line);
        if (line_len > 0 && line[line_len-1] == '\n') {
            line_len--;  // exclude the newline character
        }
        if (line_len > cols_rows) {
            cols_rows = line_len;
        }
        rows++;
    }

    // Allocate memory for the map matrix
    if (rows > 0 && cols_rows > 0) {
        map_matrix = (int **)malloc(rows * sizeof(int *));
        for (int i = 0; i < rows; i++) {
            map_matrix[i] = (int *)calloc(cols_rows, sizeof(int));
        }

        // Second pass: read the map content
        rewind(file);
        for (int i = 0; i < rows; i++) {
            if (fgets(line, sizeof(line), file)) {
                int line_len = strlen(line);
                if (line_len > 0 && line[line_len-1] == '\n') {
                    line_len--;  // exclude the newline character
                }
                
                // Copy characters to the matrix
                for (int j = 0; j < line_len && j < cols_rows; j++) {
                    map_matrix[i][j] = line[j];
                }
                // Fill remaining positions in the row with spaces
                for (int j = line_len; j < cols_rows; j++) {
                    map_matrix[i][j] = ' ';
                }
            }
        }
    }

    fclose(file);
}

// Free the dynamically allocated map matrix
void free_map_matrix() {
    if (map_matrix != NULL) {
        for (int i = 0; i < rows; i++) {
            free(map_matrix[i]);
        }
        free(map_matrix);
        map_matrix = NULL;
    }
}

// Render and save the complete frame to the display piece
void render_print() {
    FILE *fp2 = fopen("pieces/display/current_frame.txt", "w");
    if (fp2 == NULL) {
        fprintf(stderr, "Error opening pieces/display/current_frame.txt\n");
        return;
    }

    // Write the complete frame including map and pet stats
    fprintf(fp2, "==================================\n");
    fprintf(fp2, "       FUZZPET DASHBOARD        \n");
    fprintf(fp2, "==================================\n");
    
    // Get pet stats from piece manager
    char pet_name[100] = "Fuzzball";
    char hunger_str[10] = "50";
    char happiness_str[10] = "75";
    char energy_str[10] = "100";
    char level_str[10] = "1";
    
    // Get pet name
    FILE *fp = popen("./+x/piece_manager.+x fuzzpet get-state name", "r");
    if (fp) {
        if (fgets(pet_name, sizeof(pet_name), fp)) {
            pet_name[strcspn(pet_name, "\n")] = 0;  // Remove newline
        }
        pclose(fp);
    }
    
    // Get hunger
    fp = popen("./+x/piece_manager.+x fuzzpet get-state hunger", "r");
    if (fp) {
        if (fgets(hunger_str, sizeof(hunger_str), fp)) {
            hunger_str[strcspn(hunger_str, "\n")] = 0;  // Remove newline
        }
        pclose(fp);
    }
    
    // Get happiness
    fp = popen("./+x/piece_manager.+x fuzzpet get-state happiness", "r");
    if (fp) {
        if (fgets(happiness_str, sizeof(happiness_str), fp)) {
            happiness_str[strcspn(happiness_str, "\n")] = 0;  // Remove newline
        }
        pclose(fp);
    }
    
    // Get energy
    fp = popen("./+x/piece_manager.+x fuzzpet get-state energy", "r");
    if (fp) {
        if (fgets(energy_str, sizeof(energy_str), fp)) {
            energy_str[strcspn(energy_str, "\n")] = 0;  // Remove newline
        }
        pclose(fp);
    }
    
    // Get level
    fp = popen("./+x/piece_manager.+x fuzzpet get-state level", "r");
    if (fp) {
        if (fgets(level_str, sizeof(level_str), fp)) {
            level_str[strcspn(level_str, "\n")] = 0;  // Remove newline
        }
        pclose(fp);
    }
    
    // Get positions
    int pos_x = player_x;
    int pos_y = player_y;
    
    fprintf(fp2, "Pet Name: %s\n", pet_name);
    fprintf(fp2, "Hunger: %s\n", hunger_str);
    fprintf(fp2, "Happiness: %s\n", happiness_str);
    fprintf(fp2, "Energy: %s\n", energy_str);
    fprintf(fp2, "Level: %s\n", level_str);
    fprintf(fp2, "Position: (%d, %d)\n", pos_x, pos_y);
    fprintf(fp2, "\n");
    
    fprintf(fp2, "GAME MAP:\n");
    
    // Write the map matrix - careful to preserve proper formatting
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols_rows; j++) {
            fprintf(fp2, "%c", map_matrix[i][j]);
        }
        fprintf(fp2, "\n");  // Ensure proper line ending
    }
    
    fprintf(fp2, "Controls: w=up, s=down, a=left, d=right | 1=feed, 2=play, 3=sleep, 4=status, 5=evolve\n");
    fprintf(fp2, "==================================\n");

    fclose(fp2);
}

// Update map with player position and game logic
void update_map_0(int x_coords, int y_coords) {
    int new_x = player_x + x_coords;
    int new_y = player_y + y_coords;

    // Check for out-of-bounds
    if (new_x < 0 || new_x >= cols_rows - 1 || new_y < 0 || new_y >= rows - 1) {
        return; // Do nothing if out of bounds
    }

    // Check what's in the target tile
    int target_tile = map_matrix[new_y][new_x];
    
    if (target_tile == WALL_SYMBOL) {
        return; // Cannot move into walls
    } else if (target_tile == TREASURE_SYMBOL) {
        // Collect treasure
        score += 10;
        // Update pet happiness in its piece file
        char cmd[200];
        // Get current happiness
        FILE *fp = popen("./+x/piece_manager.+x fuzzpet get-state happiness", "r");
        int happiness = 75; // default
        if (fp) {
            char val[10];
            if (fgets(val, sizeof(val), fp)) {
                happiness = atoi(val);
            }
            pclose(fp);
        }
        // Increase happiness
        sprintf(cmd, "./+x/piece_manager.+x fuzzpet set-state happiness %d", happiness + 5);
        system(cmd);
    } else if (target_tile == SPACE_SYMBOL || target_tile == '.') {
        // Clear old player position
        if (map_matrix[player_y][player_x] == PLAYER_SYMBOL) {
            map_matrix[player_y][player_x] = SPACE_SYMBOL;
        }
        // Move player to new position
        player_x = new_x;
        player_y = new_y;
        map_matrix[player_y][player_x] = PLAYER_SYMBOL;
        
        // Update pet position in its piece file
        char cmd[200];
        sprintf(cmd, "./+x/piece_manager.+x fuzzpet set-state pos_x %d", player_x);
        system(cmd);
        sprintf(cmd, "./+x/piece_manager.+x fuzzpet set-state pos_y %d", player_y);
        system(cmd);
    }
    // Note: We don't move onto rock_symbol or other obstacles
}

// Update player stats based on commands
void update_player_stats() {
    // Decrease energy over time (simulated)
    char cmd[200];
    // Get current energy
    FILE *fp = popen("./+x/piece_manager.+x fuzzpet get-state energy", "r");
    int energy = 100; // default
    if (fp) {
        char val[10];
        if (fgets(val, sizeof(val), fp)) {
            energy = atoi(val);
        }
        pclose(fp);
    }
    
    // Decrease energy slightly
    if (energy > 0) {
        sprintf(cmd, "./+x/piece_manager.+x fuzzpet set-state energy %d", energy - 1);
        system(cmd);
    }
    
    // Increase hunger over time
    fp = popen("./+x/piece_manager.+x fuzzpet get-state hunger", "r");
    int hunger = 50; // default
    if (fp) {
        char val[10];
        if (fgets(val, sizeof(val), fp)) {
            hunger = atoi(val);
        }
        pclose(fp);
    }
    
    if (hunger < 100) {
        sprintf(cmd, "./+x/piece_manager.+x fuzzpet set-state hunger %d", hunger + 1);
        system(cmd);
    }
}

// Process the key command
void process_key_command(int key) {
    int x = 0, y = 0;

    switch (key) {
        case 1000: y--; break; // ARROW_UP (w)
        case 1001: y++; break; // ARROW_DOWN (s)
        case 1002: x++; break; // ARROW_RIGHT (d)
        case 1003: x--; break; // ARROW_LEFT (a)
        case 'w':
        case 'W': y--; break;  // w key
        case 's':
        case 'S': y++; break;  // s key
        case 'a':
        case 'A': x--; break;  // a key
        case 'd':
        case 'D': x++; break;  // d key
        case '1': 
            // Feed command
            {
                char cmd[200];
                // Get current hunger
                FILE *fp = popen("./+x/piece_manager.+x fuzzpet get-state hunger", "r");
                int hunger = 50; // default
                if (fp) {
                    char val[10];
                    if (fgets(val, sizeof(val), fp)) {
                        hunger = atoi(val);
                    }
                    pclose(fp);
                }
                // Decrease hunger
                int new_hunger = (hunger > 20) ? hunger - 20 : 0;
                sprintf(cmd, "./+x/piece_manager.+x fuzzpet set-state hunger %d", new_hunger);
                system(cmd);
                
                // Increase happiness
                fp = popen("./+x/piece_manager.+x fuzzpet get-state happiness", "r");
                int happiness = 75; // default
                if (fp) {
                    char val[10];
                    if (fgets(val, sizeof(val), fp)) {
                        happiness = atoi(val);
                    }
                    pclose(fp);
                }
                int new_happiness = (happiness < 95) ? happiness + 5 : 100;
                sprintf(cmd, "./+x/piece_manager.+x fuzzpet set-state happiness %d", new_happiness);
                system(cmd);
                
                // Log event to master ledger
                time_t rawtime;
                struct tm *timeinfo;
                char timestamp[100];
                time(&rawtime);
                timeinfo = localtime(&rawtime);
                strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
                
                FILE *ledger = fopen("pieces/master_ledger/master_ledger.txt", "a");
                if (ledger) {
                    fprintf(ledger, "[%s] EventFire: fed on fuzzpet | Trigger: feed_action\n", timestamp);
                    fclose(ledger);
                }
            }
            return;
        case '2':
            // Play command
            {
                char cmd[200];
                // Get current happiness
                FILE *fp = popen("./+x/piece_manager.+x fuzzpet get-state happiness", "r");
                int happiness = 75; // default
                if (fp) {
                    char val[10];
                    if (fgets(val, sizeof(val), fp)) {
                        happiness = atoi(val);
                    }
                    pclose(fp);
                }
                // Increase happiness
                int new_happiness = (happiness < 90) ? happiness + 10 : 100;
                sprintf(cmd, "./+x/piece_manager.+x fuzzpet set-state happiness %d", new_happiness);
                system(cmd);
                
                // Decrease energy
                fp = popen("./+x/piece_manager.+x fuzzpet get-state energy", "r");
                int energy = 100; // default
                if (fp) {
                    char val[10];
                    if (fgets(val, sizeof(val), fp)) {
                        energy = atoi(val);
                    }
                    pclose(fp);
                }
                int new_energy = (energy > 20) ? energy - 20 : 0;
                sprintf(cmd, "./+x/piece_manager.+x fuzzpet set-state energy %d", new_energy);
                system(cmd);
                
                // Log event to master ledger
                time_t rawtime;
                struct tm *timeinfo;
                char timestamp[100];
                time(&rawtime);
                timeinfo = localtime(&rawtime);
                strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
                
                FILE *ledger = fopen("pieces/master_ledger/master_ledger.txt", "a");
                if (ledger) {
                    fprintf(ledger, "[%s] EventFire: played on fuzzpet | Trigger: play_action\n", timestamp);
                    fclose(ledger);
                }
            }
            return;
        case '3':
            // Sleep command
            {
                char cmd[200];
                // Get current energy
                FILE *fp = popen("./+x/piece_manager.+x fuzzpet get-state energy", "r");
                int energy = 100; // default
                if (fp) {
                    char val[10];
                    if (fgets(val, sizeof(val), fp)) {
                        energy = atoi(val);
                    }
                    pclose(fp);
                }
                // Increase energy
                int new_energy = (energy < 95) ? energy + 20 : 100;
                sprintf(cmd, "./+x/piece_manager.+x fuzzpet set-state energy %d", new_energy);
                system(cmd);
                
                // Log event to master ledger
                time_t rawtime;
                struct tm *timeinfo;
                char timestamp[100];
                time(&rawtime);
                timeinfo = localtime(&rawtime);
                strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
                
                FILE *ledger = fopen("pieces/master_ledger/master_ledger.txt", "a");
                if (ledger) {
                    fprintf(ledger, "[%s] EventFire: slept on fuzzpet | Trigger: sleep_action\n", timestamp);
                    fclose(ledger);
                }
            }
            return;
        case '4':
            // Status command - just trigger a display update
            break;
        case '5':
            // Evolve command
            {
                char cmd[200];
                // Get current level
                FILE *fp = popen("./+x/piece_manager.+x fuzzpet get-state level", "r");
                int level = 1; // default
                if (fp) {
                    char val[10];
                    if (fgets(val, sizeof(val), fp)) {
                        level = atoi(val);
                    }
                    pclose(fp);
                }
                // Increase level
                int new_level = (level < 10) ? level + 1 : level; // Max level 10
                sprintf(cmd, "./+x/piece_manager.+x fuzzpet set-state level %d", new_level);
                system(cmd);
                
                // Log event to master ledger
                time_t rawtime;
                struct tm *timeinfo;
                char timestamp[100];
                time(&rawtime);
                timeinfo = localtime(&rawtime);
                strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
                
                FILE *ledger = fopen("pieces/master_ledger/master_ledger.txt", "a");
                if (ledger) {
                    fprintf(ledger, "[%s] EventFire: evolved on fuzzpet | Trigger: evolve_action\n", timestamp);
                    fclose(ledger);
                }
            }
            return;
        default: 
            if (key >= 'a' && key <= 'z') {
                // Update keyboard piece with the character typed
                char cmd[200];
                sprintf(cmd, "./+x/piece_manager.+x keyboard set-state last_key_logged '%c'", key);
                system(cmd);
            }
            // Log command received to master ledger
            time_t rawtime;
            struct tm *timeinfo;
            char timestamp[100];
            time(&rawtime);
            timeinfo = localtime(&rawtime);
            strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
            
            FILE *ledger = fopen("pieces/master_ledger/master_ledger.txt", "a");
            if (ledger) {
                fprintf(ledger, "[%s] EventFire: command_received on fuzzpet | Trigger: user_input\n", timestamp);
                fclose(ledger);
            }
            return;
    }

    if (x != 0 || y != 0) {
        update_map_0(x, y);
    }
    
    update_player_stats();
    render_print();
}

// Main function
int main(int argc, char **argv) {
    // Ensure directories exist
    system("mkdir -p pieces/keyboard pieces/display");
    
    // Initialize last history position
    FILE *history = fopen("pieces/keyboard/history.txt", "r");
    if (history) {
        fseek(history, 0, SEEK_END);
        last_history_position = ftell(history);
        fclose(history);
    } else {
        // Create directory and file if they don't exist
        history = fopen("pieces/keyboard/history.txt", "w");
        if (history) {
            fclose(history);
            last_history_position = 0;
        } else {
            return 1;
        }
    }

    // Initial map load
    load_map("map.txt");
    
    // Find player position on the map initially
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols_rows; j++) {
            if (map_matrix[i][j] == PLAYER_SYMBOL || map_matrix[i][j] == '@') {
                player_x = j;
                player_y = i;
                break;
            }
        }
    }
    
    update_player_stats();
    render_print();     // Render the initial map

    // Main game loop
    while (1) {
        // Check for new entries in keyboard history
        FILE *history = fopen("pieces/keyboard/history.txt", "r");
        if (history) {
            // Seek to last known position
            fseek(history, last_history_position, SEEK_SET);
            
            char line[200];
            while (fgets(line, sizeof(line), history)) {
                // Look for KEY_PRESSED in the line
                if (strstr(line, "KEY_PRESSED: ")) {
                    char *key_pos = strstr(line, "KEY_PRESSED: ") + 13;
                    int key = atoi(key_pos);
                    if (key > 0) {  // Valid key code
                        process_key_command(key);
                    }
                }
            }
            
            // Update position for next check
            last_history_position = ftell(history);
            fclose(history);
        }
        
        usleep(16667); // ~60 FPS
    }

    free_map_matrix(); // Clean up memory
    return 0;
}