#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAP_WIDTH 50
#define MAP_HEIGHT 20

int main() {
    // Read the world map from piece
    char game_map[MAP_HEIGHT][MAP_WIDTH+1];
    FILE *fp = fopen("peices/world/map.txt", "r");
    if (fp) {
        char line[200];
        int row = 0;
        while (fgets(line, sizeof(line), fp) && row < MAP_HEIGHT) {
            line[strcspn(line, "\n\r")] = '\0';  // Remove newline
            strncpy(game_map[row], line, MAP_WIDTH);
            // Ensure the row is filled to MAP_WIDTH
            for (int i = strlen(line); i < MAP_WIDTH; i++) {
                game_map[row][i] = '.';
            }
            game_map[row][MAP_WIDTH] = '\0';  // Null terminate
            row++;
        }
        
        // Fill remaining rows if needed
        for (int r = row; r < MAP_HEIGHT; r++) {
            for (int c = 0; c < MAP_WIDTH; c++) {
                game_map[r][c] = '.';
            }
            game_map[r][MAP_WIDTH] = '\0';
        }
        fclose(fp);
    } else {
        // Create a default map if the piece doesn't exist
        for (int r = 0; r < MAP_HEIGHT; r++) {
            for (int c = 0; c < MAP_WIDTH; c++) {
                if (r == 0 || r == MAP_HEIGHT-1 || c == 0 || c == MAP_WIDTH-1) {
                    game_map[r][c] = '#';  // Border walls
                } else if (c % 10 == 0 && r % 10 == 0) {
                    game_map[r][c] = 'T';  // Trees
                } else if (c % 15 == 5 && r % 15 == 5) {
                    game_map[r][c] = '~';  // Water
                } else if (c % 7 == 3 && r % 7 == 3) {
                    game_map[r][c] = 'R';  // Rocks
                } else {
                    game_map[r][c] = '.';  // Ground
                }
            }
            game_map[r][MAP_WIDTH] = '\0';
        }
        
        // Write to piece file
        fp = fopen("peices/world/map.txt", "w");
        if (fp) {
            for (int r = 0; r < MAP_HEIGHT; r++) {
                fprintf(fp, "%s\r", game_map[r]);
            }
            fclose(fp);
        }
    }

    // Read player position
    int player_x = 0, player_y = 0;
    fp = fopen("peices/player/player.txt", "r");
    if (fp) {
        char line[200];
        while (fgets(line, sizeof(line), fp)) {
            if (strncmp(line, "x ", 2) == 0) {
                sscanf(line + 2, "%d", &player_x);
            } else if (strncmp(line, "y ", 2) == 0) {
                sscanf(line + 2, "%d", &player_y);
            }
        }
        fclose(fp);
    } else {
        // Create default player file
        fp = fopen("peices/player/player.txt", "w");
        if (fp) {
            fprintf(fp, "x 15\r");
            fprintf(fp, "y 10\r");
            fprintf(fp, "symbol @\r");
            fprintf(fp, "health 100\r");
            fprintf(fp, "status active\r");
            fclose(fp);
        }
        player_x = 15;
        player_y = 10;
    }

    // Read entities (for completeness, we'll parse them from entities.txt if available)
    // For now, we'll just hardcode a few to test
    int entities_x[10] = {20, 25};  // Example positions
    int entities_y[10] = {12, 10};
    char entities_symbol[10] = {'Z', '&'};  // Zombie, Sheep
    int num_entities = 2;
    
    // Try to read from entities file if it exists
    fp = fopen("peices/world/entities.txt", "r");
    if (fp) {
        char line[200];
        num_entities = 0;
        while (fgets(line, sizeof(line), fp) && num_entities < 10) {
            if (line[0] != '#' && strlen(line) > 1) {  // Skip comments
                // Parse entity: x y symbol description
                int x, y;
                char symbol;
                if (sscanf(line, "%d %d %c", &x, &y, &symbol) == 3) {
                    if (x >= 0 && x < MAP_WIDTH && y >= 0 && y < MAP_HEIGHT) {
                        entities_x[num_entities] = x;
                        entities_y[num_entities] = y;
                        entities_symbol[num_entities] = symbol;
                        num_entities++;
                    }
                }
            }
        }
        fclose(fp);
    }

    // Create a display map by copying the base map
    char display_map[MAP_HEIGHT][MAP_WIDTH+1];
    for (int r = 0; r < MAP_HEIGHT; r++) {
        strcpy(display_map[r], game_map[r]);
    }
    
    // Place entities on the display map (don't overwrite player position)
    for (int i = 0; i < num_entities; i++) {
        int x = entities_x[i];
        int y = entities_y[i];
        char symbol = entities_symbol[i];
        
        if (x >= 0 && x < MAP_WIDTH && y >= 0 && y < MAP_HEIGHT) {
            // Don't overwrite the player if they're in the same position
            if (!(x == player_x && y == player_y)) {
                display_map[y][x] = symbol;
            }
        }
    }
    
    // Place the player on the display map
    if (player_x >= 0 && player_x < MAP_WIDTH && player_y >= 0 && player_y < MAP_HEIGHT) {
        display_map[player_y][player_x] = '@';
    }

    // Clear screen and print everything
    printf("\033[2J\033[H");  // Clear screen and move to home
    
    // Print everything with \r\n format for clean output
    printf("ASCII MAP:\r\n");
    printf("==================================================\r\n");  // Shorter separator
    
    // Print the map with \r\n for each line
    for (int r = 0; r < MAP_HEIGHT; r++) {
        for (int c = 0; c < MAP_WIDTH; c++) {
            putchar(display_map[r][c]);
        }
        printf("\r\n");
    }
    
    printf("==================================================\r\n");  // Shorter separator
    printf("Player pos: (%d, %d)  ", player_x, player_y);  // More compact
    printf("Opts: 0:end 1:hun 2:sta 3:act 4:inv\r\n");  // More compact
    printf("Leg: @=Plr #=Wal .=Grnd T=Tr ~=Wtr R=Rck Z=Zmb &=Shp\r\n");  // More compact
    printf("Ctl: WASD/arrows=move 0-9=menu Ctrl+C=quit\r\n");  // More compact
    
    fflush(stdout);
    
    return 0;
}