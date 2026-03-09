/*
 * playrm_module.c - Player/Runner Module (MINIMAL VERSION)
 * 
 * Responsibilities:
 * 1. Poll history.txt for input
 * 2. Call Ops based on input (WASD → move_player, X/Z → move_z)
 * 3. Trigger render after each action
 * 
 * This is the MINIMAL version - just navigation, no events/scripts yet.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

#define MAX_PATH 4096
#define MAX_LINE 256

char project_root[MAX_PATH] = ".";

/* Trim whitespace */
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
            }
        }
        fclose(kvp);
    }
}

/* Call an Op by name with arguments */
void call_op(const char* op_name, const char* arg1, const char* arg2) {
    char cmd[MAX_PATH * 2];
    snprintf(cmd, sizeof(cmd), "%s/pieces/apps/playrm/ops/+x/%s.+x %s %s > /dev/null 2>&1",
             project_root, op_name, arg1, arg2 ? arg2 : "");
    system(cmd);
}

/* Hit frame marker to trigger re-render */
void hit_frame_marker() {
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s/pieces/display/frame_changed.txt", project_root);
    FILE *f = fopen(path, "a");
    if (f) { fprintf(f, "X\n"); fclose(f); }
}

/* Write to last_response.txt for display */
void set_response(const char* msg) {
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s/pieces/apps/fuzzpet_app/fuzzpet/last_response.txt", project_root);
    FILE *f = fopen(path, "w");
    if (f) { fprintf(f, "%-57s", msg); fclose(f); }
}

int main() {
    resolve_paths();
    
    /* Initialize - hit frame marker so renderer picks us up */
    hit_frame_marker();
    set_response("Player Module Started - WASD to move, X/Z for floors");
    
    /* Main loop - poll history.txt */
    long last_pos = 0;
    struct stat st;
    char history_path[MAX_PATH];
    snprintf(history_path, sizeof(history_path), "%s/pieces/apps/fuzzpet_app/fuzzpet/history.txt", project_root);
    
    while (1) {
        if (stat(history_path, &st) == 0) {
            if (st.st_size > last_pos) {
                /* Read new keys */
                FILE *hf = fopen(history_path, "r");
                if (hf) {
                    fseek(hf, last_pos, SEEK_SET);
                    int key;
                    while (fscanf(hf, "%d", &key) == 1) {
                        /* Process key - MINIMAL: just movement */
                        if (key == 'w' || key == 'W') {
                            call_op("move_player", "selector", "up");
                            set_response("Moved up");
                        }
                        else if (key == 's' || key == 'S') {
                            call_op("move_player", "selector", "down");
                            set_response("Moved down");
                        }
                        else if (key == 'a' || key == 'A') {
                            call_op("move_player", "selector", "left");
                            set_response("Moved left");
                        }
                        else if (key == 'd' || key == 'D') {
                            call_op("move_player", "selector", "right");
                            set_response("Moved right");
                        }
                        else if (key == 'x' || key == 'X') {
                            call_op("move_z", "selector", "up");
                            set_response("Floor up");
                        }
                        else if (key == 'z' || key == 'Z') {
                            call_op("move_z", "selector", "down");
                            set_response("Floor down");
                        }
                        
                        /* Trigger re-render */
                        hit_frame_marker();
                    }
                    last_pos = ftell(hf);
                    fclose(hf);
                }
            } else if (st.st_size < last_pos) {
                last_pos = 0;  /* File was truncated */
            }
        }
        usleep(16667);  /* ~60 FPS poll rate */
    }
    
    return 0;
}
