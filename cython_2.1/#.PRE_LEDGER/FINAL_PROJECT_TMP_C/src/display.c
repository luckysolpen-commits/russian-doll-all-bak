/*** display.c - TPM Compliant Display Module ***/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAP_WIDTH 50
#define MAP_HEIGHT 20
#define MAX_ENTITIES 100

extern char game_map[][51];
extern int player_x, player_y;
extern char entities[][3];
extern int num_entities;

// Function to render the game map and entities using \r for clean updates
void renderGame() {
    // Create a copy of the map to draw on
    char display_map[MAP_HEIGHT][MAP_WIDTH+1];
    for (int r = 0; r < MAP_HEIGHT; r++) {
        strcpy(display_map[r], game_map[r]);
    }
    
    // Draw entities on the map
    for (int i = 0; i < num_entities; i++) {
        int x = (unsigned char)entities[i][0];
        int y = (unsigned char)entities[i][1];
        char symbol = entities[i][2];
        
        if (x >= 0 && x < MAP_WIDTH && y >= 0 && y < MAP_HEIGHT) {
            // Don't overwrite the player if they're in the same position
            if (!(x == player_x && y == player_y)) {
                display_map[y][x] = symbol;
            }
        }
    }
    
    // Draw the player
    if (player_x >= 0 && player_x < MAP_WIDTH && player_y >= 0 && player_y < MAP_HEIGHT) {
        display_map[player_y][player_x] = '@';
    }
    
    // Use \r\n to keep output clean and avoid staggering
    printf("\033[H");  // Move cursor to top-left corner without clearing
    
    // Print everything with \r\n format for clean output
    printf("ASCII MAP:\r\n");
    printf("================================================================================\r\n");
    
    // Print the map with \r\n for each line
    for (int r = 0; r < MAP_HEIGHT; r++) {
        for (int c = 0; c < MAP_WIDTH; c++) {
            putchar(display_map[r][c]);
        }
        printf("\r\n");  // Use \r\n to ensure clean line endings
    }
    
    printf("================================================================================\r\n");
    printf("Player position: (%d, %d)\r\n", player_x, player_y);
    printf("\r\nMENU OPTIONS:\r\n");
    printf("0: end_turn\r\n1: hunger\r\n2: state\r\n3: act\r\n4: inventory\r\n");
    printf("\r\nLEGEND: @=Player, #=Wall, .=Ground, T=Tree, ~=Water, R=Rock, Z=Zombie, &=Sheep\r\n");
    printf("CONTROLS: WASD or arrows to move, 0-9 for menu options, Ctrl+C to quit\r\n");
    
    fflush(stdout);
}

// Update the display if needed
void updateDisplay() {
    renderGame();
}