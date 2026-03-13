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
#include <pthread.h>

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
char active_target_id[64] = "selector";
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
                char *k = trim_str(line), *v = trim_str(eq + 1);
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
        if (stat(abs_p, &st) == 0 && S_ISDIR(st.st_mode)) strncpy(projects[project_count++], entry->d_name, MAX_LINE - 1);
        free(abs_p);
    }
    closedir(dir); free(path);
}

void scan_maps() {
    char *path = NULL;
    if (asprintf(&path, "%s/projects/%s/maps", project_root, current_project) == -1) return;
    DIR *dir = opendir(path);
    if (!dir) { map_count = 0; free(path); return; }
    map_count = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && map_count < MAX_ITEMS) {
        if (strstr(entry->d_name, ".txt")) strncpy(project_maps[map_count++], entry->d_name, MAX_LINE - 1);
    }
    closedir(dir); free(path);
}

void trigger_render() {
    char *cmd = NULL;
    /* render_map.+x hits the frame marker itself */
    if (asprintf(&cmd, "%s/pieces/apps/playrm/ops/+x/render_map.+x > /dev/null 2>&1", project_root) != -1) {
        system(cmd); free(cmd);
    }
}

void set_response(const char* msg) {
    char *path = NULL;
    if (asprintf(&path, "%s/pieces/apps/editor/response.txt", project_root) != -1) {
        FILE *f = fopen(path, "w"); if (f) { fprintf(f, "%-57s", msg); fclose(f); }
        free(path);
    }
}

int get_state_int_fast(const char* piece_id, const char* key) {
    char *path = NULL;
    if (asprintf(&path, "%s/projects/%s/pieces/%s/state.txt", project_root, current_project, piece_id) == -1) return -1;
    FILE *f = fopen(path, "r");
    char line[MAX_LINE]; int val = -1;
    if (f) {
        while (fgets(line, sizeof(line), f)) {
            char *eq = strchr(line, '=');
            if (eq) { *eq = '\0'; if (strcmp(trim_str(line), key) == 0) { val = atoi(trim_str(eq + 1)); break; } }
        }
        fclose(f);
    }
    free(path); return val;
}

void write_editor_state() {
    char *path = NULL;
    if (asprintf(&path, "%s/pieces/apps/player_app/manager/state.txt", project_root) != -1) {
        FILE *f = fopen(path, "w");
        if (f) { fprintf(f, "project_id=%s\nactive_target_id=%s\ncurrent_z=%d\nlast_key=%s\n", current_project, active_target_id, cursor_z, last_key_str); fclose(f); }
        free(path);
    }
    if (asprintf(&path, "%s/pieces/apps/player_app/state.txt", project_root) != -1) {
        FILE *f = fopen(path, "w"); if (!f) { free(path); return; }
        fprintf(f, "project_id=%s\ngui_focus=%d\nlast_key=%s\nselector_pos=(%d,%d,%d)\n", current_project, gui_focus_index, last_key_str, cursor_x, cursor_y, cursor_z);
        char raw_resp[MAX_LINE] = ""; char *resp_path = NULL;
        if (asprintf(&resp_path, "%s/pieces/apps/editor/response.txt", project_root) != -1) {
            FILE *rf = fopen(resp_path, "r"); if (rf) { if (fgets(raw_resp, sizeof(raw_resp), rf)) trim_str(raw_resp); fclose(rf); }
            free(resp_path);
        }
        fprintf(f, "editor_response=[RESP]: %-49s\n", raw_resp);
        char stats[MAX_LINE]; snprintf(stats, sizeof(stats), "[POS]: (%d,%d,%d) | [H-BEAT]: %d | [KEY]: %-10s", cursor_x, cursor_y, cursor_z, last_sync_idx, last_key_str);
        fprintf(f, "editor_status_2=%-57s\n", stats);
        char maps_str[MAX_LINE] = "";
        for (int i = 0; i < map_count && strlen(maps_str) < 30; i++) { if (i == map_idx) strcat(maps_str, "["); strcat(maps_str, project_maps[i]); if (i == map_idx) strcat(maps_str, "]"); strcat(maps_str, " "); }
        fprintf(f, "map_palette_view=%s\n", maps_str);
        char gly_str[MAX_LINE] = ""; const char **active_set = emoji_mode ? emoji_glyphs : ascii_glyphs;
        for (int i = 0; i < 10; i++) { if (i == glyph_idx) strcat(gly_str, "["); strcat(gly_str, active_set[i]); if (i == glyph_idx) strcat(gly_str, "]"); strcat(gly_str, " "); }
        fprintf(f, "glyph_palette_view=%s\narmed_glyph=%s\n", gly_str, active_set[glyph_idx]);
        fclose(f); free(path);
    }
}

int is_active_layout() {
    char *path = NULL;
    if (asprintf(&path, "%s/pieces/display/current_layout.txt", project_root) == -1) return 0;
    FILE *f = fopen(path, "r"); if (!f) { free(path); return 0; }
    char line[MAX_LINE];
    if (fgets(line, sizeof(line), f)) { fclose(f); int res = (strstr(line, "editor.chtpm") != NULL || strstr(line, "man-ops.chtpm") != NULL || strstr(line, "fuzz_legacy.chtpm") != NULL || strstr(line, "op-ed.chtpm") != NULL); free(path); return res; }
    fclose(f); free(path); return 0;
}

void sync_focus() {
    char *path = NULL;
    if (asprintf(&path, "%s/pieces/display/active_gui_index.txt", project_root) != -1) {
        FILE *f = fopen(path, "r");
        if (f) { int active_idx = 0; if (fscanf(f, "%d", &active_idx) == 1) { last_sync_idx = active_idx; if (active_idx > 0) gui_focus_index = active_idx; } fclose(f); }
        free(path);
    }
}

void process_key(int key) {
    if (key >= 32 && key <= 126) snprintf(last_key_str, sizeof(last_key_str), "%c", (char)key);
    else if (key == 10 || key == 13) strcpy(last_key_str, "ENTER");
    else if (key == 27) strcpy(last_key_str, "ESC");
    else if (key == 1000) strcpy(last_key_str, "LEFT");
    else if (key == 1001) strcpy(last_key_str, "RIGHT");
    else if (key == 1002) strcpy(last_key_str, "UP");
    else if (key == 1003) strcpy(last_key_str, "DOWN");

    if (key == '9' || key == 27) { strcpy(active_target_id, "selector"); set_response("Returned to Selector"); }
    if (key >= '1' && key <= '9') {
        if (key == '4') { emoji_mode = !emoji_mode; char msg[64]; snprintf(msg, sizeof(msg), "Emoji Mode: %s", emoji_mode ? "ON" : "OFF"); set_response(msg); }
        else gui_focus_index = key - '0';
    }

    if (key == 122) { /* Ctrl+Z */
        char *cmd = NULL; if (asprintf(&cmd, "%s/pieces/apps/playrm/ops/+x/undo_action.+x > /dev/null 2>&1", project_root) != -1) { system(cmd); free(cmd); set_response("Undo performed"); }
    }
    if (key == 127 || key == 8) { /* Backspace */
        if (gui_focus_index == 1 && map_count > 0) {
            char *cmd = NULL; if (asprintf(&cmd, "%s/pieces/apps/editor/ops/+x/place_tile.+x %s %d %d . > /dev/null 2>&1", project_root, project_maps[map_idx], cursor_x, cursor_y) != -1) { system(cmd); free(cmd); set_response("Tile cleared"); }
        }
    }

    if (gui_focus_index == 1) {
        if (key == 'w' || key == 'W' || key == 1002 || key == 's' || key == 'S' || key == 1003 || key == 'a' || key == 'A' || key == 1000 || key == 'd' || key == 'D' || key == 1001) {
            const char *dir = (key == 'w' || key == 'W' || key == 1002) ? "w" : (key == 's' || key == 'S' || key == 1003) ? "s" : (key == 'a' || key == 'A' || key == 1000) ? "a" : "d";
            char *cmd = NULL; if (asprintf(&cmd, "%s/pieces/apps/playrm/ops/+x/move_entity.+x %s %s %s > /dev/null 2>&1", project_root, active_target_id, dir, current_project) != -1) { system(cmd); free(cmd); }
        }
        if (strcmp(active_target_id, "selector") == 0 && (key == 10 || key == 13)) {
            char *pieces_p = NULL; if (asprintf(&pieces_p, "%s/projects/%s/pieces", project_root, current_project) != -1) {
                DIR *dir = opendir(pieces_p); if (dir) {
                    struct dirent *entry; while ((entry = readdir(dir)) != NULL) {
                        if (entry->d_name[0] == '.' || strcmp(entry->d_name, "selector") == 0) continue;
                        int tx = get_state_int_fast(entry->d_name, "pos_x"), ty = get_state_int_fast(entry->d_name, "pos_y");
                        if (tx == cursor_x && ty == cursor_y) { strncpy(active_target_id, entry->d_name, 63); char *m = NULL; if (asprintf(&m, "Selected %s", entry->d_name) != -1) { set_response(m); free(m); } break; }
                    }
                    closedir(dir);
                }
                free(pieces_p);
            }
        }
        char *s_path = NULL; if (asprintf(&s_path, "%s/projects/%s/pieces/selector/state.txt", project_root, current_project) != -1) {
            FILE *ssf = fopen(s_path, "r"); if (ssf) {
                char line[MAX_LINE]; while (fgets(line, sizeof(line), ssf)) {
                    char *eq = strchr(line, '='); if (eq) { *eq = '\0'; char *k = trim_str(line), *v = trim_str(eq + 1); if (strcmp(k, "pos_x") == 0) cursor_x = atoi(v); else if (strcmp(k, "pos_y") == 0) cursor_y = atoi(v); else if (strcmp(k, "pos_z") == 0) cursor_z = atoi(v); }
                }
                fclose(ssf);
            }
            free(s_path);
        }
        if (strcmp(active_target_id, "selector") != 0) {
            int px = get_state_int_fast(active_target_id, "pos_x"), py = get_state_int_fast(active_target_id, "pos_y");
            if (px != -1 && py != -1) {
                char *c = NULL; if (asprintf(&c, "%s/pieces/master_ledger/plugins/+x/piece_manager.+x selector set-state pos_x %d > /dev/null 2>&1", project_root, px) != -1) { system(c); free(c); }
                if (asprintf(&c, "%s/pieces/master_ledger/plugins/+x/piece_manager.+x selector set-state pos_y %d > /dev/null 2>&1", project_root, py) != -1) { system(c); free(c); }
                cursor_x = px; cursor_y = py;
            }
        }
    }
    else if (gui_focus_index == 2) { if (key == 1000 || key == 'a') { map_idx--; if (map_idx < 0) map_idx = map_count - 1; } else if (key == 1001 || key == 'd') { map_idx++; if (map_idx >= map_count) map_idx = 0; } }
    else if (gui_focus_index == 3) { if (key == 1000 || key == 'a') { glyph_idx--; if (glyph_idx < 0) glyph_idx = 9; } else if (key == 1001 || key == 'd') { glyph_idx++; if (glyph_idx >= 10) glyph_idx = 0; } }

    if (key == ' ' && gui_focus_index == 1 && map_count > 0) {
        const char **active_set = emoji_mode ? emoji_glyphs : ascii_glyphs; const char *glyph = active_set[glyph_idx];
        int is_entity = (strcmp(glyph, "@") == 0 || strcmp(glyph, "&") == 0 || strcmp(glyph, "Z") == 0 || strcmp(glyph, "T") == 0);
        char *cmd = NULL; if (is_entity) asprintf(&cmd, "%s/pieces/apps/playrm/ops/+x/create_piece.+x %s %d %d > /dev/null 2>&1", project_root, glyph, cursor_x, cursor_y);
        else asprintf(&cmd, "%s/pieces/apps/editor/ops/+x/place_tile.+x %s %d %d %s > /dev/null 2>&1", project_root, project_maps[map_idx], cursor_x, cursor_y, glyph);
        if (cmd) { system(cmd); free(cmd); if (is_entity) set_response("Piece created"); }
    }
}

int main() {
    resolve_paths(); scan_projects(); scan_maps();
    write_editor_state(); trigger_render();
    long last_pos = 0; struct stat st; char *hist_path = NULL;
    if (asprintf(&hist_path, "%s/pieces/apps/player_app/history.txt", project_root) == -1) return 1;
    while (1) {
        if (!is_active_layout()) { usleep(100000); continue; }
        sync_focus();
        if (stat(hist_path, &st) == 0) {
            if (st.st_size > last_pos) {
                FILE *hf = fopen(hist_path, "r"); if (hf) {
                    fseek(hf, last_pos, SEEK_SET); int key, processed = 0;
                    while (fscanf(hf, "%d", &key) == 1) { process_key(key); processed = 1; }
                    if (processed) { trigger_render(); write_editor_state(); }
                    last_pos = ftell(hf); fclose(hf);
                }
            } else if (st.st_size < last_pos) last_pos = 0;
        }
        usleep(16667);
    }
    free(hist_path); return 0;
}
