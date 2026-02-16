#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

#define MAP_WIDTH 20
#define MAP_HEIGHT 10

// Function to read the game world map
void read_map(char map[MAP_HEIGHT][MAP_WIDTH + 2]) {
    FILE *map_file = fopen("pieces/world/map.txt", "r");
    
    if (map_file == NULL) {
        // Create a default map if file doesn't exist
        char default_map[MAP_HEIGHT][MAP_WIDTH + 2] = {
            "####################\n",
            "#  ...............T#\n", 
            "#  ...............T#\n",
            "#  ....R@.........T#\n",
            "#  ....R..........T#\n",
            "#  ....R..........T#\n",
            "#  ....R..........T#\n",
            "#  ................#\n",
            "#                  #\n",
            "####################\n"
        };
        
        // Copy default map
        for (int i = 0; i < MAP_HEIGHT; i++) {
            strcpy(map[i], default_map[i]);
        }
        
        // Create the map file
        map_file = fopen("pieces/world/map.txt", "w");
        if (map_file) {
            for (int i = 0; i < MAP_HEIGHT; i++) {
                fputs(default_map[i], map_file);
            }
            fclose(map_file);
        }
        return;
    }
    
    // Read map from file
    for (int i = 0; i < MAP_HEIGHT; i++) {
        if (fgets(map[i], MAP_WIDTH + 3, map_file) == NULL) {
            // If we reach end of file early, fill remaining rows
            strcpy(map[i], "####################\n");
        }
    }
    
    fclose(map_file);
}

// Function to get pet position from fuzzpet piece
void get_pet_position(int *pos_x, int *pos_y) {
    // Initialize to default values
    *pos_x = 5;
    *pos_y = 5;
    
    // Use piece manager to get current position
    FILE *fp = popen("./+x/piece_manager.+x fuzzpet get-state pos_x", "r");
    if (fp) {
        char x_str[10];
        if (fgets(x_str, sizeof(x_str), fp)) {
            *pos_x = atoi(x_str);
        }
        pclose(fp);
    }
    
    fp = popen("./+x/piece_manager.+x fuzzpet get-state pos_y", "r");
    if (fp) {
        char y_str[10];
        if (fgets(y_str, sizeof(y_str), fp)) {
            *pos_y = atoi(y_str);
        }
        pclose(fp);
    }
}

// Function to update map with pet position
void update_map_with_pet(char map[MAP_HEIGHT][MAP_WIDTH + 2], int pet_x, int pet_y) {
    // Make sure coordinates are within bounds
    if (pet_x >= 0 && pet_x < MAP_WIDTH && pet_y >= 0 && pet_y < MAP_HEIGHT) {
        // Place pet symbol at its position, but don't overwrite important map elements
        char current_char = map[pet_y][pet_x];
        if (current_char == '.' || current_char == ' ') {
            map[pet_y][pet_x] = '@';  // Pet symbol
        }
    }
}

// Function to display the complete game frame
void display_game_frame() {
    char map[MAP_HEIGHT][MAP_WIDTH + 2];
    
    // Load the map
    read_map(map);
    
    // Get pet position
    int pet_x, pet_y;
    get_pet_position(&pet_x, &pet_y);
    
    // Update map with pet position
    update_map_with_pet(map, pet_x, pet_y);
    
    // Clear screen and start displaying
    printf("\033[2J\033[H");  // Clear screen and move cursor to top-left
    
    printf("==================================\n");
    printf("       FUZZPET DASHBOARD        \n");
    printf("==================================\n");
    
    // Display pet stats
    printf("Pet Name: ");
    FILE *fp = popen("./+x/piece_manager.+x fuzzpet get-state name", "r");
    if (fp) {
        char name[100];
        if (fgets(name, sizeof(name), fp)) {
            printf("%s", name);
        } else {
            printf("Fuzzball\n");
        }
        pclose(fp);
    } else {
        printf("Fuzzball\n");
    }
    
    printf("Hunger: ");
    fp = popen("./+x/piece_manager.+x fuzzpet get-state hunger", "r");
    if (fp) {
        char hunger[10];
        if (fgets(hunger, sizeof(hunger), fp)) {
            printf("%s", hunger);
        } else {
            printf("50\n");
        }
        pclose(fp);
    } else {
        printf("50\n");
    }
    
    printf("Happiness: ");
    fp = popen("./+x/piece_manager.+x fuzzpet get-state happiness", "r");
    if (fp) {
        char happiness[10];
        if (fgets(happiness, sizeof(happiness), fp)) {
            printf("%s", happiness);
        } else {
            printf("75\n");
        }
        pclose(fp);
    } else {
        printf("75\n");
    }
    
    printf("Energy: ");
    fp = popen("./+x/piece_manager.+x fuzzpet get-state energy", "r");
    if (fp) {
        char energy[10];
        if (fgets(energy, sizeof(energy), fp)) {
            printf("%s", energy);
        } else {
            printf("100\n");
        }
        pclose(fp);
    } else {
        printf("100\n");
    }
    
    printf("Level: ");
    fp = popen("./+x/piece_manager.+x fuzzpet get-state level", "r");
    if (fp) {
        char level[10];
        if (fgets(level, sizeof(level), fp)) {
            printf("%s", level);
        } else {
            printf("1\n");
        }
        pclose(fp);
    } else {
        printf("1\n");
    }
    
    printf("Position: (%d, %d)\n", pet_x, pet_y);
    printf("\n");
    
    printf("GAME MAP:\n");
    // Display the map
    for (int i = 0; i < MAP_HEIGHT; i++) {
        printf("%s", map[i]);
    }
    
    printf("\n");
    printf("CONTROLS: w=up, s=down, a=left, d=right | 1=feed, 2=play, 3=sleep, 4=status, 5=evolve\n");
    printf("Press 'q' to quit\n");
    printf("==================================\n");
}

// Function to get the last few log entries from master ledger
void display_recent_events() {
    printf("\nRecent Events:\n");
    
    // Show last 3 entries from master ledger
    FILE *ledger = fopen("pieces/master_ledger/master_ledger.txt", "r");
    if (ledger) {
        // Count total lines
        char line[500];
        int total_lines = 0;
        while (fgets(line, sizeof(line), ledger)) {
            total_lines++;
        }
        
        // Go back to beginning and skip to last few lines
        rewind(ledger);
        int lines_to_skip = total_lines > 3 ? total_lines - 3 : 0;
        int current_line = 0;
        
        while (fgets(line, sizeof(line), ledger)) {
            if (current_line >= lines_to_skip) {
                printf("  %s", line);
            }
            current_line++;
        }
        fclose(ledger);
    } else {
        printf("  No events logged yet\n");
    }
}

int main() {
    printf("Renderer Service Started\n");
    printf("Monitoring piece files for changes and updating display...\n");
    printf("Press Ctrl+C to stop renderer.\n\n");
    
    // Ensure display directory exists
    system("mkdir -p pieces/display");
    
    // Initialize current frame file
    FILE *frame_file = fopen("pieces/display/current_frame.txt", "w");
    if (frame_file) {
        fprintf(frame_file, "Display renderer initialized\n");
        fclose(frame_file);
    }
    
    // Track modification times to detect changes
    struct stat fuzzpet_stat, world_stat;
    time_t last_fuzzpet_mod = 0, last_world_mod = 0;
    
    while(1) {
        // Capture the current display frame
        FILE *temp_frame = fopen("/tmp/current_display_output.tmp", "w");
        if (temp_frame) {
            // Redirect stdout temporarily to capture display
            FILE *original_stdout = stdout;
            stdout = temp_frame;
            
            display_game_frame();
            display_recent_events();
            
            // Restore stdout
            stdout = original_stdout;
            fclose(temp_frame);
            
            // Now copy the captured frame to the official location
            system("cp /tmp/current_display_output.tmp pieces/display/current_frame.txt");
        }
        
        // Small delay to avoid excessive CPU usage
        usleep(500000); // 500ms
    }
    
    return 0;
}