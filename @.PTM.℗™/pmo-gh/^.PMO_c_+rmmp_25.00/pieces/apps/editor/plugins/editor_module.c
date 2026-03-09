/*
 * editor_module.c - Editor Module (MINIMAL VERSION)
 * 
 * Responsibilities:
 * 1. Poll history.txt for input
 * 2. Handle tab switching (1-4)
 * 3. Handle cursor movement (WASD)
 * 4. Handle tool use (SPACE = place tile)
 * 5. Trigger render after each action
 * 
 * This is the MINIMAL version - just MAP tab with paint tool.
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
int current_tab = 1;  /* 1=MAP, 2=PIECES, 3=DB, 4=EVENTS */
char selected_tile = '#';  /* Current tile to place */
int cursor_x = 5, cursor_y = 5, cursor_z = 0;

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

void call_op(const char* op_name, const char* arg1, const char* arg2, const char* arg3) {
    char cmd[MAX_PATH * 2];
    if (arg3) {
        snprintf(cmd, sizeof(cmd), "%s/pieces/apps/playrm/ops/+x/%s.+x %s %s %s > /dev/null 2>&1",
                 project_root, op_name, arg1, arg2, arg3);
    } else if (arg2) {
        snprintf(cmd, sizeof(cmd), "%s/pieces/apps/playrm/ops/+x/%s.+x %s %s > /dev/null 2>&1",
                 project_root, op_name, arg1, arg2);
    } else {
        snprintf(cmd, sizeof(cmd), "%s/pieces/apps/playrm/ops/+x/%s.+x %s > /dev/null 2>&1",
                 project_root, op_name, arg1);
    }
    system(cmd);
}

void hit_frame_marker() {
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s/pieces/display/frame_changed.txt", project_root);
    FILE *f = fopen(path, "a");
    if (f) { fprintf(f, "X\n"); fclose(f); }
}

void set_response(const char* msg) {
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s/pieces/apps/fuzzpet_app/fuzzpet/last_response.txt", project_root);
    FILE *f = fopen(path, "w");
    if (f) { fprintf(f, "%-57s", msg); fclose(f); }
}

/* Write editor state variables for parser to read */
void write_editor_state() {
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s/pieces/apps/editor/state.txt", project_root);
    FILE *f = fopen(path, "w");
    if (f) {
        fprintf(f, "selector_x=%d\n", cursor_x);
        fprintf(f, "selector_y=%d\n", cursor_y);
        fprintf(f, "selector_z=%d\n", cursor_z);
        fprintf(f, "editor_palette=[1]# [2]. [3]R [4]T [5]?\n");
        fprintf(f, "editor_tool=%s\n", current_tab == 1 ? "Paint" : "Select");
        fprintf(f, "current_tab=%d\n", current_tab);
        /* game_map will be written by render_map Op */
        fclose(f);
    }
}

int main() {
    resolve_paths();
    
    hit_frame_marker();
    set_response("Editor Module Started - 1-4: Tabs, WASD: Move, SPACE: Paint, Q: Quit");
    
    long last_pos = 0;
    struct stat st;
    char history_path[MAX_PATH];
    snprintf(history_path, sizeof(history_path), "%s/pieces/apps/fuzzpet_app/fuzzpet/history.txt", project_root);
    
    while (1) {
        if (stat(history_path, &st) == 0) {
            if (st.st_size > last_pos) {
                FILE *hf = fopen(history_path, "r");
                if (hf) {
                    fseek(hf, last_pos, SEEK_SET);
                    int key;
                    while (fscanf(hf, "%d", &key) == 1) {
                        /* Tab switching */
                        if (key >= '1' && key <= '4') {
                            current_tab = key - '0';
                            write_editor_state();
                            hit_frame_marker();
                        }
                        /* Cursor movement */
                        else if (key == 'w' || key == 'W') {
                            cursor_y--;
                            write_editor_state();
                            hit_frame_marker();
                        }
                        else if (key == 's' || key == 'S') {
                            cursor_y++;
                            write_editor_state();
                            hit_frame_marker();
                        }
                        else if (key == 'a' || key == 'A') {
                            cursor_x--;
                            write_editor_state();
                            hit_frame_marker();
                        }
                        else if (key == 'd' || key == 'D') {
                            cursor_x++;
                            write_editor_state();
                            hit_frame_marker();
                        }
                        else if (key == 'x' || key == 'X') {
                            cursor_z++;
                            write_editor_state();
                            hit_frame_marker();
                        }
                        else if (key == 'z' || key == 'Z') {
                            cursor_z--;
                            write_editor_state();
                            hit_frame_marker();
                        }
                        /* Tool use - PLACE TILE */
                        else if (key == ' ') {
                            /* Call place_tile Op */
                            char tile_str[2] = {selected_tile, '\0'};
                            char cmd[MAX_PATH];
                            snprintf(cmd, sizeof(cmd), "%s/pieces/apps/playrm/ops/+x/place_tile.+x map_001 %d %d %c > /dev/null 2>&1",
                                     project_root, cursor_x, cursor_y, selected_tile);
                            system(cmd);
                            write_editor_state();
                            hit_frame_marker();
                        }
                        /* Tile selection (number keys) */
                        else if (key >= '1' && key <= '5') {
                            const char tiles[] = "#.RT?";
                            selected_tile = tiles[key - '1'];
                            char msg[32];
                            snprintf(msg, sizeof(msg), "Tile: %c", selected_tile);
                            set_response(msg);
                        }
                        /* Quit */
                        else if (key == 'q' || key == 'Q') {
                            set_response("Quitting editor...");
                            /* Could save state here */
                        }
                        
                        hit_frame_marker();
                    }
                    last_pos = ftell(hf);
                    fclose(hf);
                }
            } else if (st.st_size < last_pos) {
                last_pos = 0;
            }
        }
        usleep(16667);
    }
    
    return 0;
}
