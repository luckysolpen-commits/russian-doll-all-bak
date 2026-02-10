/*** main.c - TPM Compliant ASCII Roguelike Game ***/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

#define MAX_PATH 256
#define MAP_WIDTH 50
#define MAP_HEIGHT 20
#define MAX_ENTITIES 100

char game_map[MAP_HEIGHT][MAP_WIDTH+1];  // +1 for null terminator
int player_x = 0, player_y = 0;
char entities[MAX_ENTITIES][3]; // x, y, symbol
int num_entities = 0;

// Include the modules directly since we're not using headers
#include "piece_manager.c"
#include "event_manager.c"
#include "display.c"
#include "world.c"
#include "player.c"
#include "control.c"

int main() {
    printf("Starting ASCII Roguelike Game (TPM C Version)...\n");
    printf("Use WASD or arrow keys to move, 0-9 for menu\n");
    printf("Press Ctrl+C to quit\n\n");
    
    // Initialize TPM components
    initPieceManager();
    initEventManager();
    
    // Ensure directories exist
    system("mkdir -p peices/world peices/player peices/input/keyboard peices/menu peices/entities");
    
    // Load map and initialize game
    loadMapFromPiece();
    initializePlayer();
    loadEntitiesFromPiece();
    
    // Start game loop
    runGameLoop();
    
    return 0;
}