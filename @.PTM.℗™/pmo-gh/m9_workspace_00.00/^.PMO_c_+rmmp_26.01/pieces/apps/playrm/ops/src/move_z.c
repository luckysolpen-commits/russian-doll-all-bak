/*
 * Op: move_z
 * Usage: ./+x/move_z.+x <piece_id> <direction>
 * Args:
 *   piece_id  - The piece to move (e.g., "player", "selector")
 *   direction - One of: up, down
 * 
 * Behavior:
 *   1. Reads piece state.txt for pos_z
 *   2. Increments (up) or decrements (down) pos_z
 *   3. Updates state.txt with new pos_z
 *   4. Hits frame_changed.txt to trigger re-render
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

#define MAX_PATH 4096
#define MAX_LINE 256

char project_root[MAX_PATH] = ".";
char pieces_dir[MAX_PATH] = "pieces";

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
                else if (strcmp(k, "pieces_dir") == 0) snprintf(pieces_dir, sizeof(pieces_dir), "%s", v);
            }
        }
        fclose(kvp);
    }
}

int get_state_int(const char* piece_id, const char* key) {
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s/pieces/world/map_01/%s/state.txt", pieces_dir, piece_id);
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

void set_state_int(const char* piece_id, const char* key, int val) {
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s/pieces/world/map_01/%s/state.txt", pieces_dir, piece_id);
    if (access(path, F_OK) != 0) {
        snprintf(path, sizeof(path), "%s/pieces/apps/fuzzpet_app/fuzzpet/state.txt", pieces_dir);
    }
    
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
    
    f = fopen(path, "w");
    if (f) {
        for (int i = 0; i < line_count; i++) fputs(lines[i], f);
        fclose(f);
    }
}

void hit_frame_marker() {
    char* path = malloc(strlen(project_root) + 32);
    sprintf(path, "%s/pieces/display/frame_changed.txt", project_root);
    FILE *f = fopen(path, "a");
    if (f) { fprintf(f, "X\n"); fclose(f); }
    free(path);
}

void log_event(const char* event, const char* piece, const char* details) {
    char* path = malloc(strlen(project_root) + 40);
    sprintf(path, "%s/pieces/master_ledger/master_ledger.txt", project_root);
    FILE *f = fopen(path, "a");
    if (f) {
        time_t rawtime; struct tm *timeinfo; char timestamp[100];
        time(&rawtime); timeinfo = localtime(&rawtime);
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
        fprintf(f, "[%s] %s: %s | %s\n", timestamp, event, piece, details);
        fclose(f);
    }
    free(path);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <piece_id> <direction>\n", argv[0]);
        fprintf(stderr, "  direction: up, down\n");
        return 1;
    }
    
    resolve_paths();
    
    const char *piece_id = argv[1];
    const char *direction = argv[2];
    
    int pos_z = get_state_int(piece_id, "pos_z");
    if (pos_z == -1) pos_z = 0;
    
    int new_z = pos_z;
    if (strcmp(direction, "up") == 0) new_z++;
    else if (strcmp(direction, "down") == 0) new_z--;
    else {
        fprintf(stderr, "Error: Unknown direction '%s'\n", direction);
        return 1;
    }
    
    set_state_int(piece_id, "pos_z", new_z);
    
    char details[64];
    snprintf(details, sizeof(details), "from z=%d to z=%d", pos_z, new_z);
    log_event("MoveZ", piece_id, details);
    
    hit_frame_marker();
    
    return 0;
}
