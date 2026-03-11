/*
 * Op: move_selector
 * Usage: ./move_selector.+x <direction> [project_id]
 * Args:
 *   direction - One of: up, down, left, right
 *   project_id - Optional, defaults to "template"
 * 
 * Moves the editor selector piece and updates state.
 * This is a SHARED OP - works for editor selector AND player selector.
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
#define MAX_CMD 16384
#define MAX_LINE 256
#define MAP_ROWS 10
#define MAP_COLS 20

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
    
    // Load from editor state
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

int get_selector_pos(const char *state_file, int *x, int *y) {
    FILE *f = fopen(state_file, "r");
    if (!f) return -1;
    
    char line[MAX_LINE];
    while (fgets(line, sizeof(line), f)) {
        char *eq = strchr(line, '=');
        if (eq) {
            *eq = '\0';
            char *k = trim_str(line);
            char *v = trim_str(eq + 1);
            if (strcmp(k, "pos_x") == 0) *x = atoi(v);
            else if (strcmp(k, "pos_y") == 0) *y = atoi(v);
        }
    }
    fclose(f);
    return 0;
}

void set_selector_pos(const char *state_file, int x, int y) {
    char lines[20][MAX_LINE];
    int count = 0;
    int found_x = 0, found_y = 0;
    
    FILE *f = fopen(state_file, "r");
    if (f) {
        while (fgets(lines[count], sizeof(lines[0]), f) && count < 19) {
            char *eq = strchr(lines[count], '=');
            if (eq) {
                *eq = '\0';
                char *k = trim_str(lines[count]);
                char *v = trim_str(eq + 1);
                if (strcmp(k, "pos_x") == 0) {
                    snprintf(lines[count], sizeof(lines[count]), "pos_x=%d\n", x);
                    found_x = 1;
                } else if (strcmp(k, "pos_y") == 0) {
                    snprintf(lines[count], sizeof(lines[count]), "pos_y=%d\n", y);
                    found_y = 1;
                } else {
                    *eq = '=';
                }
            }
            count++;
        }
        fclose(f);
    }
    
    if (!found_x && count < 20) {
        snprintf(lines[count++], sizeof(lines[0]), "pos_x=%d\n", x);
    }
    if (!found_y && count < 20) {
        snprintf(lines[count++], sizeof(lines[0]), "pos_y=%d\n", y);
    }
    
    f = fopen(state_file, "w");
    if (f) {
        for (int i = 0; i < count; i++) {
            fputs(lines[i], f);
        }
        fclose(f);
    }
}

void hit_frame_marker() {
    char path[MAX_CMD];
    snprintf(path, sizeof(path), "%s/pieces/display/frame_changed.txt", project_root);
    FILE *f = fopen(path, "a");
    if (f) { fprintf(f, "X\n"); fclose(f); }
}

void hit_editor_view_marker() {
    char path[MAX_CMD];
    snprintf(path, sizeof(path), "%s/pieces/apps/editor/view_changed.txt", project_root);
    FILE *f = fopen(path, "a");
    if (f) { fprintf(f, "X\n"); fclose(f); }
}

void log_event(const char* event, const char* details) {
    char path[MAX_CMD];
    snprintf(path, sizeof(path), "%s/pieces/master_ledger/master_ledger.txt", project_root);
    FILE *f = fopen(path, "a");
    if (f) {
        fprintf(f, "SelectorMove: %s\n", details);
        fclose(f);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <direction> [project_id]\n", argv[0]);
        fprintf(stderr, "  direction: up, down, left, right\n");
        return 1;
    }
    
    resolve_paths();
    
    const char *direction = argv[1];
    if (argc > 2) {
        snprintf(current_project, sizeof(current_project), "%s", argv[2]);
    }
    
    // Get selector position from project's selector piece
    char *selector_dir = NULL;
    if (asprintf(&selector_dir, "%s/projects/%s/pieces/selector", project_root, current_project) == -1) return 1;
    mkdir(selector_dir, 0755);

    char *selector_state = NULL;
    if (asprintf(&selector_state, "%s/state.txt", selector_dir) == -1) { free(selector_dir); return 1; }
    
    int pos_x = 5, pos_y = 5;
    get_selector_pos(selector_state, &pos_x, &pos_y);
    
    // Calculate new position
    int new_x = pos_x;
    int new_y = pos_y;
    
    if (strcmp(direction, "up") == 0) new_y--;
    else if (strcmp(direction, "down") == 0) new_y++;
    else if (strcmp(direction, "left") == 0) new_x--;
    else if (strcmp(direction, "right") == 0) new_x++;
    else {
        fprintf(stderr, "Error: Unknown direction '%s'\n", direction);
        free(selector_dir); free(selector_state);
        return 1;
    }
    
    // Bounds checking (editor map is typically 10x20)
    if (new_x < 0) new_x = 0;
    if (new_x >= MAP_COLS) new_x = MAP_COLS - 1;
    if (new_y < 0) new_y = 0;
    if (new_y >= MAP_ROWS) new_y = MAP_ROWS - 1;
    
    // Update position and ensure on_map=1/type=selector
    set_selector_pos(selector_state, new_x, new_y);
    
    // Ensure on_map and type are in state.txt
    FILE *f = fopen(selector_state, "a");
    if (f) {
        // We append to ensure they exist, though set_selector_pos should really handle it properly.
        // For now, let's just make sure they are there if not already.
        // Actually, let's just rewrite it to be clean.
    }

    // Rewrite state file cleanly
    f = fopen(selector_state, "w");
    if (f) {
        fprintf(f, "pos_x=%d\n", new_x);
        fprintf(f, "pos_y=%d\n", new_y);
        fprintf(f, "pos_z=0\n");
        fprintf(f, "on_map=1\n");
        fprintf(f, "type=selector\n");
        fclose(f);
    }
    
    free(selector_dir); free(selector_state);
    
    // Also update editor state.txt for UI display
    char editor_state[MAX_CMD];
    snprintf(editor_state, sizeof(editor_state),
             "%s/pieces/apps/editor/state.txt", project_root);
    
    // Update cursor_x, cursor_y in editor state
    FILE *es = fopen(editor_state, "r");
    char lines[20][MAX_LINE];
    int count = 0;
    if (es) {
        while (fgets(lines[count], sizeof(lines[0]), es) && count < 19) {
            char *eq = strchr(lines[count], '=');
            if (eq) {
                *eq = '\0';
                char *k = trim_str(lines[count]);
                char *v = trim_str(eq + 1);
                if (strcmp(k, "selector_pos") == 0) {
                    snprintf(lines[count], sizeof(lines[count]), "selector_pos=(%d,%d,0)\n", new_x, new_y);
                } else {
                    *eq = '=';
                }
            }
            count++;
        }
        fclose(es);
        
        es = fopen(editor_state, "w");
        if (es) {
            for (int i = 0; i < count; i++) {
                fputs(lines[i], es);
            }
            fclose(es);
        }
    }
    
    // Log and signal
    char details[MAX_CMD];
    snprintf(details, sizeof(details), "from (%d,%d) to (%d,%d) project=%s",
             pos_x, pos_y, new_x, new_y, current_project);
    log_event("MoveSelector", details);
    
    hit_frame_marker();
    hit_editor_view_marker();
    
    printf("Selector moved to (%d,%d)\n", new_x, new_y);
    return 0;
}
