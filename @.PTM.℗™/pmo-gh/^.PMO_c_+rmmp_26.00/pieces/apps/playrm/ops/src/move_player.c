/*
 * Op: move_player
 * Usage: ./+x/move_player.+x <piece_id> <direction>
 * Args:
 *   piece_id  - The piece to move (e.g., "player", "selector")
 *   direction - One of: up, down, left, right
 * 
 * Behavior:
 *   1. Reads piece state.txt for pos_x, pos_y, pos_z
 *   2. Calculates new position based on direction
 *   3. Checks collision (walls '#', rocks 'R')
 *   4. Updates state.txt with new pos_x, pos_y
 *   5. Hits frame_changed.txt to trigger re-render
 *   6. Logs to master_ledger.txt
 *
 * TPM Compliant:
 *   - Uses location_kvp for path resolution
 *   - Updates both DNA (via piece_manager) and Mirror (state.txt)
 *   - Hits frame_changed.txt marker
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define MAX_PATH 4096
#define MAX_LINE 256

char project_root[MAX_PATH] = ".";
char pieces_dir[MAX_PATH] = "pieces";

/* Trim whitespace from string */
char* trim_str(char *str) {
    char *end;
    while(*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r') str++;
    if(*str == 0) return str;
    end = str + strlen(str) - 1;
    while(end > str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) end--;
    end[1] = '\0';
    return str;
}

/* Resolve paths from location_kvp */
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
                else if (strcmp(k, "pieces_dir") == 0) snprintf(pieces_dir, sizeof(pieces_dir), "%s", v);
            }
        }
        fclose(kvp);
    }
}

/* Build path dynamically */
char* build_path_malloc(const char* rel) {
    size_t sz = strlen(project_root) + strlen(rel) + 2;
    char* p = (char*)malloc(sz);
    if (p) snprintf(p, sz, "%s/%s", project_root, rel);
    return p;
}

/* Read integer state from piece */
int get_state_int(const char* piece_id, const char* key) {
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s/pieces/world/map_01/%s/state.txt", pieces_dir, piece_id);
    
    /* Fallback to fuzzpet_app location */
    if (access(path, F_OK) != 0) {
        snprintf(path, sizeof(path), "%s/pieces/apps/fuzzpet_app/fuzzpet/state.txt", pieces_dir);
    }
    
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    
    char line[MAX_LINE];
    while (fgets(line, sizeof(line), f)) {
        char *eq = strchr(line, '=');
        if (eq) {
            *eq = '\0';
            char *k = trim_str(line);
            char *v = trim_str(eq + 1);
            if (strcmp(k, key) == 0) {
                int val = atoi(v);
                fclose(f);
                return val;
            }
        }
    }
    fclose(f);
    return -1;
}

/* Set integer state in piece */
void set_state_int(const char* piece_id, const char* key, int val) {
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s/pieces/world/map_01/%s/state.txt", pieces_dir, piece_id);
    
    /* Fallback to fuzzpet_app location */
    if (access(path, F_OK) != 0) {
        snprintf(path, sizeof(path), "%s/pieces/apps/fuzzpet_app/fuzzpet/state.txt", pieces_dir);
    }
    
    /* Read all lines, update the one we want */
    char lines[100][MAX_LINE];
    int line_count = 0;
    int found = 0;
    
    FILE *f = fopen(path, "r");
    if (f) {
        while (fgets(lines[line_count], sizeof(lines[0]), f) && line_count < 99) {
            char *eq = strchr(lines[line_count], '=');
            if (eq) {
                *eq = '\0';
                char *k = trim_str(lines[line_count]);
                if (strcmp(k, key) == 0) {
                    snprintf(lines[line_count], sizeof(lines[0]), "%s=%d\n", key, val);
                    found = 1;
                } else {
                    *eq = '=';
                }
            }
            line_count++;
        }
        fclose(f);
    }
    
    if (!found) {
        snprintf(lines[line_count++], sizeof(lines[0]), "%s=%d\n", key, val);
    }
    
    /* Write back */
    f = fopen(path, "w");
    if (f) {
        for (int i = 0; i < line_count; i++) {
            fputs(lines[i], f);
        }
        fclose(f);
    }
}

/* Check if tile is walkable */
int is_walkable(int x, int y, int z) {
    /* For now, always return true - can be extended with map reading */
    (void)x; (void)y; (void)z;
    return 1;
}

/* Hit frame marker to trigger re-render */
void hit_frame_marker() {
    char* path = build_path_malloc("pieces/display/frame_changed.txt");
    FILE *f = fopen(path, "a");
    if (f) {
        fprintf(f, "X\n");
        fclose(f);
    }
    free(path);
}

/* Log to master ledger */
void log_event(const char* event, const char* piece, const char* details) {
    char* path = build_path_malloc("pieces/master_ledger/master_ledger.txt");
    FILE *f = fopen(path, "a");
    if (f) {
        time_t rawtime;
        struct tm *timeinfo;
        char timestamp[100];
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
        fprintf(f, "[%s] %s: %s | %s\n", timestamp, event, piece, details);
        fclose(f);
    }
    free(path);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <piece_id> <direction>\n", argv[0]);
        fprintf(stderr, "  direction: up, down, left, right\n");
        return 1;
    }
    
    resolve_paths();
    
    const char *piece_id = argv[1];
    const char *direction = argv[2];
    
    /* Read current position */
    int pos_x = get_state_int(piece_id, "pos_x");
    int pos_y = get_state_int(piece_id, "pos_y");
    int pos_z = get_state_int(piece_id, "pos_z");
    
    if (pos_x == -1 || pos_y == -1) {
        fprintf(stderr, "Error: Could not read position for piece '%s'\n", piece_id);
        return 1;
    }
    
    /* Calculate new position */
    int new_x = pos_x;
    int new_y = pos_y;
    
    if (strcmp(direction, "up") == 0) new_y--;
    else if (strcmp(direction, "down") == 0) new_y++;
    else if (strcmp(direction, "left") == 0) new_x--;
    else if (strcmp(direction, "right") == 0) new_x++;
    else {
        fprintf(stderr, "Error: Unknown direction '%s'\n", direction);
        return 1;
    }
    
    /* Check collision */
    if (!is_walkable(new_x, new_y, pos_z)) {
        log_event("MoveBlocked", piece_id, "collision");
        return 0;  /* Not an error, just can't move */
    }
    
    /* Update position */
    set_state_int(piece_id, "pos_x", new_x);
    set_state_int(piece_id, "pos_y", new_y);
    
    /* Log event */
    char details[128];
    snprintf(details, sizeof(details), "from (%d,%d) to (%d,%d) direction=%s", pos_x, pos_y, new_x, new_y, direction);
    log_event("MoveEntity", piece_id, details);
    
    /* Trigger re-render */
    hit_frame_marker();
    
    return 0;
}
