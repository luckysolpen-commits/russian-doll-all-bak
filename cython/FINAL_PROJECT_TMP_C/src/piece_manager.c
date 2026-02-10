/*** piece_manager.c - TPM Compliant Piece Manager ***/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Initialize piece manager
void initPieceManager() {
    // Create piece directories if they don't exist
    system("mkdir -p peices/world peices/player peices/menu peices/entities");
}

// Update a value in a piece file
void updatePieceValue(const char* category, const char* name, const char* key, const char* value) {
    char filename[256];
    snprintf(filename, sizeof(filename), "peices/%s/%s.txt", category, name);
    
    FILE* fp = fopen(filename, "r+");
    if (!fp) {
        // Create the file if it doesn't exist
        fp = fopen(filename, "w");
        fclose(fp);
        fp = fopen(filename, "r+");
        if (!fp) return;
    }
    
    // Read all lines into memory
    char lines[1000][256];
    int line_count = 0;
    char line[256];
    
    while (fgets(line, sizeof(line), fp) && line_count < 1000) {
        strcpy(lines[line_count], line);
        line_count++;
    }
    
    fclose(fp);
    
    // Modify the line with the matching key
    int found = 0;
    for (int i = 0; i < line_count; i++) {
        if (strncmp(lines[i], key, strlen(key)) == 0 && lines[i][strlen(key)] == ' ') {
            snprintf(lines[i], sizeof(lines[i]), "%s %s\n", key, value);
            found = 1;
            break;
        }
    }
    
    // If key was not found, add it at the end
    if (!found && line_count < 1000) {
        snprintf(lines[line_count], sizeof(lines[line_count]), "%s %s\n", key, value);
        line_count++;
    }
    
    // Write all lines back to the file
    fp = fopen(filename, "w");
    if (fp) {
        for (int i = 0; i < line_count; i++) {
            fputs(lines[i], fp);
        }
        fclose(fp);
    }
}