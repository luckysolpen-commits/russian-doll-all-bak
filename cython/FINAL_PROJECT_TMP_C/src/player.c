/*** player.c - TPM Compliant Player Module ***/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int player_x, player_y;

// Initialize player from piece
void initializePlayer() {
    // Load player state from piece
    FILE *fp = fopen("peices/player/player.txt", "r");
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
        // Default starting position
        player_x = 15;  // Default to the spawn point from original entities
        player_y = 10;
    }
    
    // Ensure player is within bounds
    if (player_x < 0) player_x = 0;
    if (player_x >= MAP_WIDTH) player_x = MAP_WIDTH - 1;
    if (player_y < 0) player_y = 0;
    if (player_y >= MAP_HEIGHT) player_y = MAP_HEIGHT - 1;
    
    // Update player piece with current position
    fp = fopen("peices/player/player.txt", "w");
    if (fp) {
        fprintf(fp, "x %d\n", player_x);
        fprintf(fp, "y %d\n", player_y);
        fprintf(fp, "symbol @\n");
        fprintf(fp, "health 100\n");
        fprintf(fp, "status active\n");
        fclose(fp);
    }
    
    printf("Player initialized at (%d, %d)\n", player_x, player_y);
}

// Move player if possible
int movePlayer(int dx, int dy) {
    int new_x = player_x + dx;
    int new_y = player_y + dy;
    
    // Check bounds
    if (new_x < 0 || new_x >= MAP_WIDTH || new_y < 0 || new_y >= MAP_HEIGHT) {
        return 0;  // Cannot move
    }
    
    // Check if destination is walkable
    extern int isWalkable(int x, int y);
    if (!isWalkable(new_x, new_y)) {
        return 0;  // Cannot move
    }
    
    // Update player position
    player_x = new_x;
    player_y = new_y;
    
    // Update player piece with new position
    FILE *fp = fopen("peices/player/player.txt", "w");
    if (fp) {
        fprintf(fp, "x %d\n", player_x);
        fprintf(fp, "y %d\n", player_y);
        fprintf(fp, "symbol @\n");
        fprintf(fp, "health 100\n");
        fprintf(fp, "status active\n");
        fclose(fp);
    }
    
    return 1;  // Move successful
}