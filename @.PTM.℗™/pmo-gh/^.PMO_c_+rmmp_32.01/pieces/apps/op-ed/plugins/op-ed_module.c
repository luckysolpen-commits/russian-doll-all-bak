/*
 * editor_module.c - Advanced RMMP Editor (Phase 2 FINAL: Synced Focus & Debug)
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <ctype.h>

#define MAX_PATH 4096
#define MAX_CMD 16384
#define MAX_LINE 1024
#define MAX_ITEMS 50

char project_root[MAX_PATH] = ".";
char current_project[MAX_LINE] = "template";

/* Project Browser */
char projects[MAX_ITEMS][MAX_LINE];
int project_count = 0;
int project_idx = 0;

/* Map Palette */
char project_maps[MAX_ITEMS][MAX_LINE];
int map_count = 0;
int map_idx = 0;

/* Glyph Palette */
const char *ascii_glyphs[] = {"#", ".", "R", "T", "@", "&", "Z", "X", "?", "!"};
const char *emoji_glyphs[] = {"🧱", "🟩", "🌲", "🏰", "🎯", "🐶", "🧟", "💰", "🏠", "🔥"};
int glyph_idx = 0;
int emoji_mode = 0;

/* State */
int gui_focus_index = 1;
int cursor_x = 5, cursor_y = 5, cursor_z = 0;
char last_key_str[32] = "None";
int last_sync_idx = 0;

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
}

void scan_projects() {
    char *path = NULL;
    if (asprintf(&path, "%s/projects", project_root) == -1) return;
    DIR *dir = opendir(path);
    if (!dir) { free(path); return; }
    project_count = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && project_count < MAX_ITEMS) {
        if (entry->d_name[0] == '.') continue;
        char *abs_p = NULL;
        if (asprintf(&abs_p, "%s/%s", path, entry->d_name) == -1) continue;
        struct stat st;
        if (stat(abs_p, &st) == 0 && S_ISDIR(st.st_mode)) {
            strncpy(projects[project_count++], entry->d_name, MAX_LINE - 1);
        }
        free(abs_p);
    }
    closedir(dir);
    free(path);
}

void scan_maps() {
    char *path = NULL;
    if (asprintf(&path, "%s/projects/%s/maps", project_root, current_project) == -1) return;
    DIR *dir = opendir(path);
    if (!dir) { map_count = 0; free(path); return; }
    map_count = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && map_count < MAX_ITEMS) {
        if (strstr(entry->d_name, ".txt")) {
            strncpy(project_maps[map_count++], entry->d_name, MAX_LINE - 1);
        }
    }
    closedir(dir);
    free(path);
}

void trigger_render() {
    char cmd[MAX_CMD];
    snprintf(cmd, sizeof(cmd), "%s/pieces/apps/playrm/ops/+x/render_map.+x > /dev/null 2>&1", project_root);
    system(cmd);
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

void set_response(const char* msg) {
    char path[MAX_CMD];
    snprintf(path, sizeof(path), "%s/pieces/apps/editor/response.txt", project_root);
    FILE *f = fopen(path, "w");
    if (f) { fprintf(f, "%-57s", msg); fclose(f); }
}

void write_editor_state() {
    char path[MAX_CMD];
    /* TPM COMPLIANT: Use unified player_app state path */
    snprintf(path, sizeof(path), "%s/pieces/apps/player_app/state.txt", project_root);
    FILE *f = fopen(path, "w");
    if (!f) return;

    fprintf(f, "project_id=%s\n", current_project);
    fprintf(f, "gui_focus=%d\n", gui_focus_index);
    fprintf(f, "last_key=%s\n", last_key_str);
    fprintf(f, "selector_pos=(%d,%d,%d)\n", cursor_x, cursor_y, cursor_z);

    char raw_resp[MAX_LINE] = "";
    char resp_path[MAX_CMD];
    snprintf(resp_path, sizeof(resp_path), "%s/pieces/apps/editor/response.txt", project_root);
    FILE *rf = fopen(resp_path, "r");
    if (rf) { fgets(raw_resp, sizeof(raw_resp), rf); fclose(rf); }
    char resp_line[MAX_LINE];
    snprintf(resp_line, sizeof(resp_line), "[RESP]: %-49s", trim_str(raw_resp));
    fprintf(f, "editor_response=%s\n", resp_line);

    char stats[MAX_LINE];
    snprintf(stats, sizeof(stats), "[POS]: (%d,%d,%d) | [H-BEAT]: %d | [KEY]: %-10s", cursor_x, cursor_y, cursor_z, last_sync_idx, last_key_str);
    fprintf(f, "editor_status_2=%-57s\n", stats);

    char maps_str[MAX_LINE] = "";
    if (map_count == 0) strcpy(maps_str, "[No Maps]");
    else {
        for (int i = 0; i < map_count && strlen(maps_str) < 30; i++) {
            if (i == map_idx) strcat(maps_str, "[");
            strcat(maps_str, project_maps[i]);
            if (i == map_idx) strcat(maps_str, "]");
            strcat(maps_str, " ");
        }
    }
    fprintf(f, "map_palette_view=%s\n", maps_str);

    char gly_str[MAX_LINE] = "";
    const char **active_set = emoji_mode ? emoji_glyphs : ascii_glyphs;
    for (int i = 0; i < 10; i++) {
        if (i == glyph_idx) strcat(gly_str, "[");
        strcat(gly_str, active_set[i]);
        if (i == glyph_idx) strcat(gly_str, "]");
        strcat(gly_str, " ");
    }
    fprintf(f, "glyph_palette_view=%s\n", gly_str);
    fprintf(f, "armed_glyph=%s\n", active_set[glyph_idx]);

    fclose(f);
}

int is_active_layout() {
    char *path = NULL;
    if (asprintf(&path, "%s/pieces/display/current_layout.txt", project_root) == -1) return 0;
    FILE *f = fopen(path, "r");
    if (!f) { free(path); return 0; }
    char line[MAX_LINE];
    if (fgets(line, sizeof(line), f)) {
        fclose(f);
        int res = (strstr(line, "editor.chtpm") != NULL || strstr(line, "op-ed.chtpm") != NULL);
        free(path);
        return res;
    }
    fclose(f); free(path);
    return 0;
}

void sync_focus() {
    char path[MAX_CMD];
    snprintf(path, sizeof(path), "%s/pieces/display/active_gui_index.txt", project_root);
    FILE *f = fopen(path, "r");
    if (f) {
        int active_idx = 0;
        if (fscanf(f, "%d", &active_idx) == 1) {
            last_sync_idx = active_idx;
            if (active_idx > 0) gui_focus_index = active_idx;
        }
        fclose(f);
    }
}

int main() {
    resolve_paths();
    scan_projects();
    scan_maps();
    
    char path[MAX_CMD];
    /* TPM COMPLIANT: Use unified player_app manager path */
    snprintf(path, sizeof(path), "%s/pieces/apps/player_app/manager/state.txt", project_root);
    FILE *f_mgr = fopen(path, "w");
    if (f_mgr) {
        fprintf(f_mgr, "project_id=%s\n", current_project);
        fprintf(f_mgr, "active_target_id=selector\n");
        fprintf(f_mgr, "current_z=0\n");
        fprintf(f_mgr, "last_key=None\n");
        fclose(f_mgr);
    }

    write_editor_state();
    trigger_render();
    hit_frame_marker();
    
    long last_pos = 0;
    struct stat st;
    
    while (1) {
        if (!is_active_layout()) { usleep(100000); continue; }
        sync_focus();

        /* TPM COMPLIANT: Dynamically resolve history path based on current project */
        char history_path[MAX_CMD];
        if (strcmp(current_project, "template") != 0) {
            snprintf(history_path, sizeof(history_path), "%s/projects/%s/history.txt", project_root, current_project);
        } else {
            snprintf(history_path, sizeof(history_path), "%s/pieces/apps/player_app/history.txt", project_root);
        }

        if (stat(history_path, &st) == 0) {
            if (st.st_size > last_pos) {
                FILE *hf = fopen(history_path, "r");
                if (hf) {
                    fseek(hf, last_pos, SEEK_SET);
                    int key;
                    while (fscanf(hf, "%d", &key) == 1) {
                        if (key >= 32 && key <= 126) snprintf(last_key_str, sizeof(last_key_str), "%c", (char)key);
                        else if (key == 1000) strcpy(last_key_str, "LEFT");
                        else if (key == 1001) strcpy(last_key_str, "RIGHT");
                        else if (key == 1002) strcpy(last_key_str, "UP");
                        else if (key == 1003) strcpy(last_key_str, "DOWN");

                        if (key >= '1' && key <= '9') {
                            if (key == '4') {
                                emoji_mode = !emoji_mode;
                                char msg[64];
                                snprintf(msg, sizeof(msg), "Emoji Mode: %s", emoji_mode ? "ON" : "OFF");
                                set_response(msg);
                            } else {
                                gui_focus_index = key - '0';
                            }
                        }

                        // Ctrl+Z Undo
                        if (key == 122) {  // Ctrl+Z ASCII
                            char cmd[MAX_CMD];
                            snprintf(cmd, sizeof(cmd), "%s/pieces/apps/playrm/ops/+x/undo_action.+x > /dev/null 2>&1", project_root);
                            system(cmd);
                            set_response("Undo performed");
                        }

                        // Backspace - Destructive Editing
                        if (key == 127 || key == 8) {  // Backspace or Ctrl+H
                            if (gui_focus_index == 1 && map_count > 0) {
                                // Revert tile to default '.'
                                char cmd[MAX_CMD];
                                snprintf(cmd, sizeof(cmd), "%s/pieces/apps/editor/ops/+x/place_tile.+x %s %d %d . > /dev/null 2>&1",
                                         project_root, project_maps[map_idx], cursor_x, cursor_y);
                                system(cmd);
                                set_response("Tile cleared");
                            }
                        }

                        if (gui_focus_index == 1) {
                            /* TPM COMPLIANT: Use SHARED move_player op with 'selector' target */
                            if (key == 'w' || key == 'W' || key == 1002) {
                                char cmd[MAX_CMD];
                                snprintf(cmd, sizeof(cmd), "%s/pieces/apps/playrm/ops/+x/move_player.+x selector up > /dev/null 2>&1", project_root);
                                system(cmd);
                            }
                            else if (key == 's' || key == 'S' || key == 1003) {
                                char cmd[MAX_CMD];
                                snprintf(cmd, sizeof(cmd), "%s/pieces/apps/playrm/ops/+x/move_player.+x selector down > /dev/null 2>&1", project_root);
                                system(cmd);
                            }
                            else if (key == 'a' || key == 'A' || key == 1000) {
                                char cmd[MAX_CMD];
                                snprintf(cmd, sizeof(cmd), "%s/pieces/apps/playrm/ops/+x/move_player.+x selector left > /dev/null 2>&1", project_root);
                                system(cmd);
                            }
                            else if (key == 'd' || key == 'D' || key == 1001) {
                                char cmd[MAX_CMD];
                                snprintf(cmd, sizeof(cmd), "%s/pieces/apps/playrm/ops/+x/move_player.+x selector right > /dev/null 2>&1", project_root);
                                system(cmd);
                            }
                            // Sync local cursor vars from selector state
                            char selector_state_path[MAX_CMD];
                            snprintf(selector_state_path, sizeof(selector_state_path),
                                     "%s/projects/%s/pieces/selector/state.txt", project_root, current_project);
                            FILE *ssf = fopen(selector_state_path, "r");
                            if (ssf) {
                                char line[MAX_LINE];
                                while (fgets(line, sizeof(line), ssf)) {
                                    char *eq = strchr(line, '=');
                                    if (eq) {
                                        *eq = '\0';
                                        char *k = trim_str(line);
                                        char *v = trim_str(eq + 1);

                                        if (strcmp(k, "pos_x") == 0) cursor_x = atoi(v);
                                        else if (strcmp(k, "pos_y") == 0) cursor_y = atoi(v);
                                        else if (strcmp(k, "pos_z") == 0) cursor_z = atoi(v);
                                    }
                                }
                                fclose(ssf);
                            }
                        }
                        else if (gui_focus_index == 2) {
                            if (key == 1000) { map_idx--; if (map_idx < 0) map_idx = map_count - 1; }
                            else if (key == 1001) { map_idx++; if (map_idx >= map_count) map_idx = 0; }
                        }
                        else if (gui_focus_index == 3) {
                            if (key == 1000) { glyph_idx--; if (glyph_idx < 0) glyph_idx = 9; }
                            else if (key == 1001) { glyph_idx++; if (glyph_idx >= 10) glyph_idx = 0; }
                        }

                        if (key == ' ' && gui_focus_index == 1 && map_count > 0) {
                            const char **active_set = emoji_mode ? emoji_glyphs : ascii_glyphs;
                            const char *glyph = active_set[glyph_idx];
                            
                            // Check if glyph is an entity type
                            int is_entity = (strcmp(glyph, "@") == 0 || strcmp(glyph, "&") == 0 || 
                                            strcmp(glyph, "Z") == 0 || strcmp(glyph, "T") == 0);
                            
                            char cmd[MAX_CMD];
                            if (is_entity) {
                                // Entity glyph - create piece using SHARED create_piece op
                                snprintf(cmd, sizeof(cmd), "%s/pieces/apps/playrm/ops/+x/create_piece.+x %s %d %d > /dev/null 2>&1",
                                         project_root, glyph, cursor_x, cursor_y);
                                set_response("Piece created");
                            } else {
                                /* TPM COMPLIANT: Use SHARED playrm/ops/place_tile.+x */
                                snprintf(cmd, sizeof(cmd), "%s/pieces/apps/playrm/ops/+x/place_tile.+x %s %d %d %s > /dev/null 2>&1",
                                         project_root, project_maps[map_idx], cursor_x, cursor_y, glyph);
                            }
                            system(cmd);
                        }
                    }
                    trigger_render();
                    write_editor_state();
                    hit_frame_marker();
                    hit_editor_view_marker();
                    last_pos = ftell(hf);
                    fclose(hf);
                }
            } else if (st.st_size < last_pos) last_pos = 0;
        }
        usleep(16667);
    }
    return 0;
}
