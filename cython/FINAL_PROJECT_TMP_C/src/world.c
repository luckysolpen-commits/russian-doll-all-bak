/*** world.c - TPM Compliant World Module ***/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define MAP_WIDTH 50
#define MAP_HEIGHT 20
#define MAX_ENTITIES 100

extern char game_map[][51];
extern int num_entities;
extern char entities[][3];

// Forward declarations to prevent implicit declaration warnings
void createDefaultMap();
void createDefaultEntities();

// Load map from world piece
void loadMapFromPiece() {
    FILE *fp = fopen("peices/world/map.txt", "r");
    if (!fp) {
        printf("ERROR: Could not open world map piece\n");
        // Create default map
        createDefaultMap();
        return;
    }
    
    char line[200];
    int row = 0;
    
    while (fgets(line, sizeof(line), fp) && row < MAP_HEIGHT) {
        if (line[0] != '#' && strlen(line) > 1) {  // Skip comments
            line[strcspn(line, "\n\r")] = 0;  // Remove newline
            strncpy(game_map[row], line, MAP_WIDTH);
            // Ensure the row is filled to MAP_WIDTH
            for (int i = strlen(line); i < MAP_WIDTH; i++) {
                game_map[row][i] = '.';
            }
            game_map[row][MAP_WIDTH] = '\0';  // Null terminate
            row++;
        }
    }
    
    fclose(fp);
    
    // Fill remaining rows if needed
    for (int r = row; r < MAP_HEIGHT; r++) {
        for (int c = 0; c < MAP_WIDTH; c++) {
            game_map[r][c] = '.';
        }
        game_map[r][MAP_WIDTH] = '\0';
    }
    
    printf("World map loaded from piece: %d rows\n", row);
}

// Create a default map if the piece doesn't exist
void createDefaultMap() {
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
    FILE *fp = fopen("peices/world/map.txt", "w");
    if (fp) {
        fprintf(fp, "# Generated world map\n");
        for (int r = 0; r < MAP_HEIGHT; r++) {
            fprintf(fp, "%s\n", game_map[r]);
        }
        fclose(fp);
    }
}

// Load entities from world piece
void loadEntitiesFromPiece() {
    FILE *fp = fopen("peices/world/entities.txt", "r");
    if (!fp) {
        printf("INFO: No entities file found, using defaults\n");
        // Create default entities
        createDefaultEntities();
        return;
    }
    
    char line[200];
    num_entities = 0;
    
    while (fgets(line, sizeof(line), fp) && num_entities < MAX_ENTITIES) {
        if (line[0] != '#' && strlen(line) > 1) {  // Skip comments
            // Parse entity: x y symbol description
            int x, y;
            char symbol;
            if (sscanf(line, "%d %d %c", &x, &y, &symbol) == 3) {
                if (x >= 0 && x < MAP_WIDTH && y >= 0 && y < MAP_HEIGHT) {
                    entities[num_entities][0] = (char)x;
                    entities[num_entities][1] = (char)y;
                    entities[num_entities][2] = symbol;
                    num_entities++;
                }
            }
        }
    }
    
    fclose(fp);
    printf("Loaded %d entities from piece\n", num_entities);
}

// Create default entities if the piece doesn't exist
void createDefaultEntities() {
    // Example: zombie at (20,12) and sheep at (25,10)
    if (num_entities < MAX_ENTITIES) {
        entities[num_entities][0] = 20;  // x
        entities[num_entities][1] = 12;  // y
        entities[num_entities][2] = 'Z'; // symbol
        num_entities++;
    }
    if (num_entities < MAX_ENTITIES) {
        entities[num_entities][0] = 25;  // x
        entities[num_entities][1] = 10;  // y
        entities[num_entities][2] = '&'; // symbol
        num_entities++;
    }
    
    // Write to piece file
    FILE *fp = fopen("peices/world/entities.txt", "w");
    if (fp) {
        fprintf(fp, "# Entity placements\n");
        fprintf(fp, "# Format: x y symbol description\n");
        for (int i = 0; i < num_entities; i++) {
            fprintf(fp, "%d %d %c default_entity\n", 
                   (unsigned char)entities[i][0], 
                   (unsigned char)entities[i][1], 
                   entities[i][2]);
        }
        fclose(fp);
    }
}

// Check if position is walkable
int isWalkable(int x, int y) {
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) {
        return 0;  // Out of bounds
    }
    
    char tile = game_map[y][x];
    return (tile == '.' || tile == '~' || tile == 'T' || tile == 'R');  // Walkable tiles
}