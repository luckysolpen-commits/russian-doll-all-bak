/*
 * place_tile.c - Editor Op for placing tiles on project maps
 * Usage: place_tile.+x <map_file> <x> <y> <glyph>
 * 
 * This is the EDITOR's place_tile - it modifies PROJECT maps,
 * NOT fuzzpet_app world maps.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

#define MAX_PATH 4096
#define MAX_LINE 1024
#define MAP_HEIGHT 20
#define MAP_WIDTH 40

char project_root[MAX_PATH] = ".";
char current_project[MAX_LINE] = "template";

char* trim_str(char *str) {
    char *end;
    if(!str) return str;
    while(isspace((unsigned char)*str)) str++;
    if(*str == 0) return str;
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

void resolve_paths() {
    FILE *kvp = fopen("pieces/locations/location_kvp", "r");
    if (kvp) {
        char line[MAX_LINE];
        while (fgets(line, sizeof(line), kvp)) {
            char *eq = strchr(line, '=');
            if (eq) {
                *eq = '\0';
                char *k = trim_str(line);
                char *v = trim_str(eq + 1);
                if (strcmp(k, "project_root") == 0) snprintf(project_root, sizeof(project_root), "%s", v);
            }
        }
        fclose(kvp);
    }
    
    // Load current project from editor state
    FILE *state = fopen("pieces/apps/editor/state.txt", "r");
    if (state) {
        char line[MAX_LINE];
        while (fgets(line, sizeof(line), state)) {
            char *eq = strchr(line, '=');
            if (eq) {
                *eq = '\0';
                char *k = trim_str(line);
                char *v = trim_str(eq + 1);
                if (strcmp(k, "project_id") == 0) snprintf(current_project, sizeof(current_project), "%s", v);
            }
        }
        fclose(state);
    }
}

int place_tile(const char *map_file, int x, int y, const char *glyph) {
    char map_path[MAX_PATH];
    
    // Check if map_file is absolute or relative
    if (map_file[0] == '/') {
        snprintf(map_path, sizeof(map_path), "%s", map_file);
    } else {
        snprintf(map_path, sizeof(map_path), "%s/projects/%s/maps/%s", project_root, current_project, map_file);
    }
    
    // Read existing map
    FILE *f = fopen(map_path, "r");
    if (!f) {
        // Map doesn't exist, create default
        f = fopen(map_path, "w");
        if (!f) {
            fprintf(stderr, "Error: Cannot create map file %s\n", map_path);
            return 1;
        }
        // Write default empty map
        for (int row = 0; row < MAP_HEIGHT; row++) {
            for (int col = 0; col < MAP_WIDTH; col++) {
                fputc('.', f);
            }
            fputc('\n', f);
        }
        fclose(f);
        f = fopen(map_path, "r");
        if (!f) {
            fprintf(stderr, "Error: Cannot read newly created map\n");
            return 1;
        }
    }
    
    // Read map into buffer
    char map_buffer[MAP_HEIGHT][MAP_WIDTH + 1];
    memset(map_buffer, '.', sizeof(map_buffer));
    
    int row = 0;
    while (row < MAP_HEIGHT && fgets(map_buffer[row], sizeof(map_buffer[row]) + 1, f)) {
        // Remove newline
        char *nl = strchr(map_buffer[row], '\n');
        if (nl) *nl = '\0';
        row++;
    }
    fclose(f);
    
    // Place the tile
    if (x >= 0 && x < MAP_WIDTH && y >= 0 && y < MAP_HEIGHT) {
        map_buffer[y][x] = glyph[0];
    }
    
    // Write map back
    f = fopen(map_path, "w");
    if (!f) {
        fprintf(stderr, "Error: Cannot write map file %s\n", map_path);
        return 1;
    }
    
    for (int i = 0; i < MAP_HEIGHT; i++) {
        fprintf(f, "%s\n", map_buffer[i]);
    }
    fclose(f);
    
    // Log to project ledger (optional, for undo support)
    char ledger_path[MAX_PATH];
    snprintf(ledger_path, sizeof(ledger_path), "%s/projects/%s/config/editor_history.ledger", project_root, current_project);
    FILE *ledger = fopen(ledger_path, "a");
    if (ledger) {
        fprintf(ledger, "PLACE_TILE|%s|%d|%d|.|%c\n", map_file, x, y, glyph[0]);
        fclose(ledger);
    }
    
    printf("Placed '%c' at (%d,%d) on %s\n", glyph[0], x, y, map_file);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 5) {
        fprintf(stderr, "Usage: %s <map_file> <x> <y> <glyph>\n", argv[0]);
        return 1;
    }
    
    resolve_paths();
    
    const char *map_file = argv[1];
    int x = atoi(argv[2]);
    int y = atoi(argv[3]);
    const char *glyph = argv[4];
    
    return place_tile(map_file, x, y, glyph);
}
