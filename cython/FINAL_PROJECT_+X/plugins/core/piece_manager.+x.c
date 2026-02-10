#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#define MAX_PIECES 100
#define MAX_LINE_LEN 1024
#define MAX_PATH_LEN 256

// Piece structure
typedef struct {
    char id[MAX_PATH_LEN];
    char path[MAX_PATH_LEN];
    char name[256];
    char data[MAX_LINE_LEN][256];
    int num_data_lines;
} Piece;

Piece pieces[MAX_PIECES];
int num_pieces = 0;

// Helper function to trim whitespace
char* trim(char* str) {
    char* end;
    // Trim leading space
    while(*str == ' ') str++;
    if(strlen(str) == 0)  return str;
    // Trim trailing space
    end = str + strlen(str) - 1;
    while(end > str && (*end == ' ' || *end == '\n' || *end == '\r')) end--;
    *(end+1) = '\0';
    return str;
}

// Load a single piece file
void loadPieceFile(const char* piece_path, const char* piece_id) {
    if (num_pieces >= MAX_PIECES) return;
    
    FILE* fp = fopen(piece_path, "r");
    if (!fp) return;
    
    Piece* piece = &pieces[num_pieces];
    strcpy(piece->path, piece_path);
    strcpy(piece->id, piece_id);
    
    // Extract name from path - get the last folder name
    strcpy(piece->name, piece_path);
    char* slash = strrchr(piece->name, '/');
    if (slash) strcpy(piece->name, slash + 1);
    // Remove .txt extension if present
    char* dot = strrchr(piece->name, '.');
    if (dot) *dot = '\0';
    
    piece->num_data_lines = 0;
    char line[MAX_LINE_LEN];
    
    while (fgets(line, MAX_LINE_LEN, fp) && piece->num_data_lines < MAX_LINE_LEN) {
        line[strcspn(line, "\n\r")] = 0; // Remove newline
        char* trimmed = trim(line);
        if (strlen(trimmed) > 0 && trimmed[0] != '#') {
            strcpy(piece->data[piece->num_data_lines], trimmed);
            piece->num_data_lines++;
        }
    }
    
    fclose(fp);
    num_pieces++;
}

// Scan directory for piece files
void scanPiecesDir(const char* dir_path) {
    system("ls -la peices/ > /dev/null 2>&1 || mkdir -p peices/");
    
    // For this implementation, we'll directly load known piece files
    char world_path[MAX_PATH_LEN];
    snprintf(world_path, MAX_PATH_LEN, "peices/world/world.txt");
    if (access(world_path, F_OK) == 0) {
        loadPieceFile(world_path, "world");
    }
    
    char player_path[MAX_PATH_LEN];
    snprintf(player_path, MAX_PATH_LEN, "peices/player/player.txt");
    if (access(player_path, F_OK) == 0) {
        loadPieceFile(player_path, "player");
    }
    
    char keyboard_path[MAX_PATH_LEN];
    snprintf(keyboard_path, MAX_PATH_LEN, "peices/keyboard/keyboard.txt");
    if (access(keyboard_path, F_OK) == 0) {
        loadPieceFile(keyboard_path, "keyboard");
    }
    
    char menu_path[MAX_PATH_LEN];
    snprintf(menu_path, MAX_PATH_LEN, "peices/menu/menu.txt");
    if (access(menu_path, F_OK) == 0) {
        loadPieceFile(menu_path, "menu");
    }
    
    char event_path[MAX_PATH_LEN];
    snprintf(event_path, MAX_PATH_LEN, "peices/clock_events/clock_events.txt");
    if (access(event_path, F_OK) == 0) {
        loadPieceFile(event_path, "clock_events");
    }
}

// Load all pieces from the pieces directory
void loadAllPieces() {
    num_pieces = 0;
    scanPiecesDir("peices");
}

// Get piece by name
Piece* getPiece(const char* name) {
    for (int i = 0; i < num_pieces; i++) {
        if (strcmp(pieces[i].name, name) == 0) {
            return &pieces[i];
        }
    }
    return NULL;
}

// Get value for a key in a piece
char* getPieceValue(Piece* piece, const char* key) {
    if (!piece) return NULL;
    
    for (int i = 0; i < piece->num_data_lines; i++) {
        if (strncmp(piece->data[i], key, strlen(key)) == 0 && 
            piece->data[i][strlen(key)] == ' ') {
            return piece->data[i] + strlen(key) + 1;
        }
    }
    return NULL;
}

// Update a value in a piece file
void updatePieceValue(const char* category, const char* name, const char* key, const char* value) {
    char piece_path[MAX_PATH_LEN];
    snprintf(piece_path, MAX_PATH_LEN, "peices/%s/%s.txt", category, name);
    
    // Create directory if needed
    char dir_path[MAX_PATH_LEN];
    snprintf(dir_path, MAX_PATH_LEN, "peices/%s", category);
    char cmd[MAX_PATH_LEN + 50];
    snprintf(cmd, sizeof(cmd), "mkdir -p %s", dir_path);
    system(cmd);
    
    FILE* fp = fopen(piece_path, "r");
    char temp_content[MAX_LINE_LEN][256];
    int line_count = 0;
    char line[MAX_LINE_LEN];
    
    int updated = 0;
    if (fp) {
        while (fgets(line, MAX_LINE_LEN, fp) && line_count < MAX_LINE_LEN) {
            line[strcspn(line, "\n\r")] = 0;
            char* trimmed = trim(line);
            if (strlen(trimmed) > 0 && trimmed[0] != '#') {
                if (strncmp(trimmed, key, strlen(key)) == 0 && trimmed[strlen(key)] == ' ') {
                    snprintf(temp_content[line_count], sizeof(temp_content[line_count]), "%s %s", key, value);
                    updated = 1;
                } else {
                    strcpy(temp_content[line_count], trimmed);
                }
            } else {
                strcpy(temp_content[line_count], line);
            }
            line_count++;
        }
        fclose(fp);
    }
    
    // If key was not found, add it at the end
    if (!updated) {
        snprintf(temp_content[line_count], sizeof(temp_content[line_count]), "%s %s", key, value);
        line_count++;
    }
    
    // Write all lines back to the file
    fp = fopen(piece_path, "w");
    if (fp) {
        for (int i = 0; i < line_count; i++) {
            fprintf(fp, "%s\n", temp_content[i]);
        }
        fclose(fp);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <command> [args]\n", argv[0]);
        printf("Commands: get_piece_state <piece_name>, load_pieces, update_piece_state <cat> <name> <key> <val>\n");
        return 1;
    }
    
    if (strcmp(argv[1], "load_pieces") == 0) {
        loadAllPieces();
        printf("Loaded %d pieces\n", num_pieces);
    } else if (strcmp(argv[1], "get_piece_state") == 0 && argc == 3) {
        loadAllPieces(); // Reload to get latest
        Piece* piece = getPiece(argv[2]);
        if (piece) {
            for (int i = 0; i < piece->num_data_lines; i++) {
                printf("%s\n", piece->data[i]);
            }
        } else {
            printf("Piece %s not found\n", argv[2]);
        }
    } else if (strcmp(argv[1], "update_piece_state") == 0 && argc == 6) {
        updatePieceValue(argv[2], argv[3], argv[4], argv[5]);
    } else {
        printf("Unknown command or insufficient arguments\n");
        return 1;
    }
    
    return 0;
}