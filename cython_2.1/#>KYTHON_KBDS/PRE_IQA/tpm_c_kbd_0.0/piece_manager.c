/*** piece_manager.c - TPM Compliant Piece Manager ***/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#define MAX_PIECES 100
#define MAX_PATH_LEN 256
#define MAX_LINE_LEN 1024

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
    strcpy(piece->name, strrchr(piece_path, '/') - 1);
    char* slash = strrchr(piece->name, '/');
    if (slash) strcpy(piece->name, slash + 1);
    
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
    DIR* dir = opendir(dir_path);
    if (!dir) return;
    
    struct dirent* entry;
    char path[MAX_PATH_LEN];
    
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue; // Skip hidden files/dirs
        
        snprintf(path, MAX_PATH_LEN, "%s/%s", dir_path, entry->d_name);
        
        struct stat path_stat;
        stat(path, &path_stat);
        
        if (S_ISDIR(path_stat.st_mode)) {
            // Recursively scan subdirectory
            scanPiecesDir(path);
        } else if (S_ISREG(path_stat.st_mode) && strstr(entry->d_name, ".txt")) {
            // This is a piece file - construct piece ID
            char piece_id[MAX_PATH_LEN];
            // Extract category and piece name from the path
            strcpy(piece_id, path + strlen("peices/")); // Remove "peices/" prefix
            loadPieceFile(path, piece_id);
        }
    }
    
    closedir(dir);
}

// Load all pieces from the pieces directory
void loadAllPieces() {
    num_pieces = 0;
    scanPiecesDir("peices");
}

// Get piece by category and name
Piece* getPiece(const char* category, const char* name) {
    char search_id[MAX_PATH_LEN];
    snprintf(search_id, MAX_PATH_LEN, "%s/%s/%s.txt", category, name, name);
    
    for (int i = 0; i < num_pieces; i++) {
        if (strcmp(pieces[i].id, search_id) == 0) {
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
    snprintf(piece_path, MAX_PATH_LEN, "peices/%s/%s/%s.txt", category, name, name);
    
    // Read current file
    FILE* fp = fopen(piece_path, "r");
    if (!fp) return;
    
    char lines[MAX_LINE_LEN][256];
    int num_lines = 0;
    char line[256];
    
    while (fgets(line, sizeof(line), fp) && num_lines < MAX_LINE_LEN) {
        strcpy(lines[num_lines], line);
        num_lines++;
    }
    fclose(fp);
    
    // Update or add the key-value pair
    int found = 0;
    for (int i = 0; i < num_lines; i++) {
        if (strncmp(lines[i], key, strlen(key)) == 0 && lines[i][strlen(key)] == ' ') {
            snprintf(lines[i], sizeof(lines[i]), "%s %s\n", key, value);
            found = 1;
            break;
        }
    }
    
    // If key not found, add it to the end
    if (!found) {
        snprintf(lines[num_lines], sizeof(lines[num_lines]), "%s %s\n", key, value);
        num_lines++;
    }
    
    // Write back to file
    fp = fopen(piece_path, "w");
    if (fp) {
        for (int i = 0; i < num_lines; i++) {
            fputs(lines[i], fp);
        }
        fclose(fp);
    }
}

// Initialize the piece manager
void initPieceManager() {
    loadAllPieces();
}