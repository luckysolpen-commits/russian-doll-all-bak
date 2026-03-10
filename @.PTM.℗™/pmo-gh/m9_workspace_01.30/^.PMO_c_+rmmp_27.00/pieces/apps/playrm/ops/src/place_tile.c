/*
 * Op: place_tile
 * Usage: ./+x/place_tile.+x <map_id> <x> <y> <tile_char>
 * Args:
 *   map_id    - The map file (e.g., "map_001")
 *   x, y      - Position on map
 *   tile_char - Character to place (e.g., "#", ".", "R")
 * 
 * Behavior:
 *   1. Reads map file (projects/{project}/maps/{map_id}_z{z}.txt)
 *   2. Replaces character at (x, y)
 *   3. Writes back to map file
 *   4. Hits frame_changed.txt
 *   5. Logs to master_ledger.txt
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>

#define MAX_PATH 4096
#define MAX_LINE 256
#define MAX_MAP_LINES 100

char project_root[MAX_PATH] = ".";
char current_project[MAX_PATH] = "demo_1";  /* Default project */
int current_z = 0;

char* trim_str(char *str) {
    char *end;
    while(*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r') str++;
    if(*str == 0) return str;
    end = str + strlen(str) - 1;
    while(end > str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) end--;
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
}

void hit_frame_marker() {
    char *path = NULL;
    if (asprintf(&path, "%s/pieces/display/frame_changed.txt", project_root) != -1) {
        FILE *f = fopen(path, "a");
        if (f) { fprintf(f, "X\n"); fclose(f); }
        free(path);
    }
}

void log_event(const char* event, const char* details) {
    char *path = NULL;
    if (asprintf(&path, "%s/pieces/master_ledger/master_ledger.txt", project_root) != -1) {
        FILE *f = fopen(path, "a");
        if (f) {
            time_t rawtime; struct tm *timeinfo; char timestamp[100];
            time(&rawtime); timeinfo = localtime(&rawtime);
            strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
            fprintf(f, "[%s] %s: %s\n", timestamp, event, details);
            fclose(f);
        }
        free(path);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 5) {
        fprintf(stderr, "Usage: %s <map_id> <x> <y> <tile_char>\n", argv[0]);
        return 1;
    }
    
    resolve_paths();
    
    const char *map_id = argv[1];
    int x = atoi(argv[2]);
    int y = atoi(argv[3]);
    char tile = argv[4][0];
    
    /* Build map file path */
    char *map_path = NULL;
    if (asprintf(&map_path, "%s/projects/%s/maps/%s_z%d.txt",
             project_root, current_project, map_id, current_z) == -1) return 1;
    
    /* Read map into memory */
    char lines[MAX_MAP_LINES][MAX_LINE];
    int line_count = 0;
    
    FILE *f = fopen(map_path, "r");
    if (!f) {
        fprintf(stderr, "Error: Could not open %s\n", map_path);
        free(map_path);
        return 1;
    }
    
    while (fgets(lines[line_count], sizeof(lines[0]), f) && line_count < MAX_MAP_LINES - 1) {
        lines[line_count][strcspn(lines[line_count], "\n\r")] = 0;
        line_count++;
    }
    fclose(f);
    
    if (y >= 0 && y < line_count) {
        int line_len = strlen(lines[y]);
        if (x >= 0 && x < line_len) {
            /* Place tile */
            lines[y][x] = tile;
            
            /* Write back */
            f = fopen(map_path, "w");
            if (f) {
                for (int i = 0; i < line_count; i++) {
                    fprintf(f, "%s\n", lines[i]);
                }
                fclose(f);
            }
            
            /* Log and signal */
            char details[128];
            snprintf(details, sizeof(details), "map=%s x=%d y=%d tile=%c", map_id, x, y, tile);
            log_event("PlaceTile", details);
            hit_frame_marker();
        }
    }
    
    free(map_path);
    return 0;
}
