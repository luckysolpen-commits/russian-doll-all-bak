/*
 * op-ed_module.c - Advanced RMMP Editor (Unified Trait Edition)
 * CPU-SAFE: Signal handling + fork/exec/waitpid pattern
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
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <ctype.h>
#include <pthread.h>

#define MAX_PATH 4096
#define MAX_CMD 16384
#define MAX_LINE 1024
#define MAX_ITEMS 50

/* CPU-SAFE: Global shutdown flag and signal handler */
static volatile sig_atomic_t g_shutdown = 0;

static void handle_sigint(int sig) {
    (void)sig;
    g_shutdown = 1;
}

/* CPU-SAFE: Helper to run external commands with fork/exec/waitpid */
static int run_command(const char* cmd) {
    pid_t pid = fork();
    if (pid == 0) {
        /* Child: redirect stdout/stderr to /dev/null */
        freopen("/dev/null", "w", stdout);
        freopen("/dev/null", "w", stderr);
        execl("/bin/sh", "/bin/sh", "-c", cmd, (char *)NULL);
        _exit(127);
    } else if (pid > 0) {
        /* Parent: wait for child */
        int status;
        waitpid(pid, &status, 0);
        return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    }
    return -1;
}

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
int stage_map_idx = -1; /* Tracks which map is actually active for editing */

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
    if (!dir) { 
        map_count = 1;
        strcpy(project_maps[0], "ADD");
        free(path); return; 
    }
    map_count = 0;
    strcpy(project_maps[map_count++], "ADD");
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && map_count < MAX_ITEMS) {
        if (strstr(entry->d_name, ".txt")) strncpy(project_maps[map_count++], entry->d_name, MAX_LINE - 1);
    }
    closedir(dir); free(path);
}

void trigger_render() {
    char *cmd = NULL;
    if (asprintf(&cmd, "%s/pieces/apps/playrm/ops/+x/render_map.+x > /dev/null 2>&1", project_root) != -1) {
        run_command(cmd); free(cmd);
    }
}

void hit_frame_marker() {
    char *path = NULL;
    if (asprintf(&path, "%s/pieces/display/frame_changed.txt", project_root) != -1) {
        FILE *f = fopen(path, "a");
        if (f) { fprintf(f, "M\n"); fclose(f); }
        free(path);
    }
}

void set_response(const char* msg) {
    char *path = NULL;
    if (asprintf(&path, "%s/pieces/apps/editor/response.txt", project_root) != -1) {
        FILE *f = fopen(path, "w"); if (f) { fprintf(f, "%-57s", msg); fclose(f); }
        free(path);
    }
}

void sync_current_map() {
    if (stage_map_idx < 1 || stage_map_idx >= map_count) return;
    char *path = NULL;
    if (asprintf(&path, "%s/pieces/apps/player_app/manager/state.txt", project_root) != -1) {
        FILE *f = fopen(path, "r");
        char lines[50][MAX_LINE];
        int line_count = 0, found = 0;
        if (f) {
            while (fgets(lines[line_count], sizeof(lines[0]), f) && line_count < 49) {
                char *eq = strchr(lines[line_count], '=');
                if (eq) {
                    *eq = '\0';
                    if (strcmp(trim_str(lines[line_count]), "current_map") == 0) {
                        snprintf(lines[line_count], sizeof(lines[0]), "current_map=%s\n", project_maps[stage_map_idx]);
                        found = 1;
                    } else *eq = '=';
                }
                line_count++;
            }
            fclose(f);
        }
        if (!found && line_count < 50) snprintf(lines[line_count++], sizeof(lines[0]), "current_map=%s\n", project_maps[stage_map_idx]);
        f = fopen(path, "w");
        if (f) { for (int i = 0; i < line_count; i++) fputs(lines[i], f); fclose(f); }
        free(path);
    }
}

void save_selector_state() {
    char *path = NULL;
    if (asprintf(&path, "%s/projects/%s/pieces/selector/state.txt", project_root, current_project) != -1) {
        FILE *f = fopen(path, "w");
        if (f) {
            fprintf(f, "pos_x=%d\npos_y=%d\npos_z=%d\nactive=1\nicon=X\ntype=selector\n", cursor_x, cursor_y, cursor_z);
            fclose(f);
        }
        free(path);
    }
}

void create_new_map() {
    int max_num = 0;
    char *path = NULL;
    if (asprintf(&path, "%s/projects/%s/maps", project_root, current_project) == -1) return;
    DIR *dir = opendir(path);
    if (dir) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strncmp(entry->d_name, "map_", 4) == 0) {
                int num = atoi(entry->d_name + 4);
                if (num > max_num) max_num = num;
            }
        }
        closedir(dir);
    }
    char new_map[MAX_LINE]; snprintf(new_map, sizeof(new_map), "map_%04d.txt", max_num + 1);
    char *full_path = NULL;
    if (asprintf(&full_path, "%s/%s", path, new_map) != -1) {
        FILE *f = fopen(full_path, "w");
        if (f) {
            for (int y = 0; y < 10; y++) {
                for (int x = 0; x < 20; x++) fputc('.', f);
                fputc('\n', f);
            }
            fclose(f); set_response("New Map Created");
        }
        free(full_path);
    }
    free(path); scan_maps();
    for (int i = 1; i < map_count; i++) {
        if (strcmp(project_maps[i], new_map) == 0) { map_idx = i; break; }
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
        /* Read existing content to preserve fields like current_map */
        FILE *f = fopen(path, "r");
        char lines[50][MAX_LINE];
        int line_count = 0, found_proj = 0, found_target = 0, found_z = 0, found_key = 0;
        if (f) {
            while (fgets(lines[line_count], sizeof(lines[0]), f) && line_count < 49) {
                char *eq = strchr(lines[line_count], '=');
                if (eq) {
                    *eq = '\0';
                    char *k = trim_str(lines[line_count]);
                    if (strcmp(k, "project_id") == 0) { snprintf(lines[line_count], sizeof(lines[0]), "project_id=%s\n", current_project); found_proj = 1; }
                    else if (strcmp(k, "active_target_id") == 0) { snprintf(lines[line_count], sizeof(lines[0]), "active_target_id=%s\n", active_target_id); found_target = 1; }
                    else if (strcmp(k, "current_z") == 0) { snprintf(lines[line_count], sizeof(lines[0]), "current_z=%d\n", cursor_z); found_z = 1; }
                    else if (strcmp(k, "last_key") == 0) { snprintf(lines[line_count], sizeof(lines[0]), "last_key=%s\n", last_key_str); found_key = 1; }
                    else *eq = '=';
                }
                line_count++;
            }
            fclose(f);
        }
        if (!found_proj && line_count < 50) snprintf(lines[line_count++], sizeof(lines[0]), "project_id=%s\n", current_project);
        if (!found_target && line_count < 50) snprintf(lines[line_count++], sizeof(lines[0]), "active_target_id=%s\n", active_target_id);
        if (!found_z && line_count < 50) snprintf(lines[line_count++], sizeof(lines[0]), "current_z=%d\n", cursor_z);
        if (!found_key && line_count < 50) snprintf(lines[line_count++], sizeof(lines[0]), "last_key=%s\n", last_key_str);
        f = fopen(path, "w");
        if (f) { for (int i = 0; i < line_count; i++) fputs(lines[i], f); fclose(f); }
        free(path);
    }
    /* Write selector state for rendering */
    char *sel_path = NULL;
    if (asprintf(&sel_path, "%s/projects/%s/pieces/selector/state.txt", project_root, current_project) != -1) {
        FILE *sf = fopen(sel_path, "w");
        if (sf) {
            fprintf(sf, "name=Selector\ntype=selector\npos_x=%d\npos_y=%d\npos_z=%d\non_map=1\n", cursor_x, cursor_y, cursor_z);
            fclose(sf);
        }
        free(sel_path);
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
    if (fgets(line, sizeof(line), f)) { fclose(f); int res = (strstr(line, "op-ed.chtpm") != NULL || strstr(line, "editor.chtpm") != NULL); free(path); return res; }
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
    if (key >= 32 && key <= 126) {
        if (key == ' ') strcpy(last_key_str, "SPACE");
        else snprintf(last_key_str, sizeof(last_key_str), "%c", (char)key);
    }
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
        char *cmd = NULL; if (asprintf(&cmd, "%s/pieces/apps/playrm/ops/+x/undo_action.+x > /dev/null 2>&1", project_root) != -1) { run_command(cmd); free(cmd); set_response("Undo performed"); }
    }
    if (key == 127 || key == 8) { /* Backspace */
        if (gui_focus_index == 1 && stage_map_idx > 0 && stage_map_idx < map_count) {
            char *cmd = NULL; if (asprintf(&cmd, "%s/pieces/apps/editor/ops/+x/place_tile.+x %s %d %d . > /dev/null 2>&1", project_root, project_maps[stage_map_idx], cursor_x, cursor_y) != -1) { run_command(cmd); free(cmd); set_response("Tile cleared"); }
        }
    }

    if (gui_focus_index == 1) {
        if (key == 'w' || key == 'W' || key == 1002 || key == 's' || key == 'S' || key == 1003 || key == 'a' || key == 'A' || key == 1000 || key == 'd' || key == 'D' || key == 1001) {
            const char *dir = (key == 'w' || key == 'W' || key == 1002) ? "w" : (key == 's' || key == 'S' || key == 1003) ? "s" : (key == 'a' || key == 'A' || key == 1000) ? "a" : "d";
            if (strcmp(active_target_id, "selector") == 0) {
                /* Move selector cursor directly */
                if (strcmp(dir, "w") == 0) cursor_y--; else if (strcmp(dir, "s") == 0) cursor_y++; else if (strcmp(dir, "a") == 0) cursor_x--; else if (strcmp(dir, "d") == 0) cursor_x++;
                if (cursor_x < 0) cursor_x = 0; if (cursor_x >= 100) cursor_x = 99; if (cursor_y < 0) cursor_y = 0; if (cursor_y >= 100) cursor_y = 99;
                save_selector_state();
            } else {
                /* Move entity */
                char *cmd = NULL; if (asprintf(&cmd, "%s/pieces/apps/playrm/ops/+x/move_entity.+x %s %s %s > /dev/null 2>&1", project_root, active_target_id, dir, current_project) != -1) { run_command(cmd); free(cmd); }
                int px = get_state_int_fast(active_target_id, "pos_x"), py = get_state_int_fast(active_target_id, "pos_y");
                if (px != -1 && py != -1) { cursor_x = px; cursor_y = py; save_selector_state(); }
            }
        }
        if (strcmp(active_target_id, "selector") == 0 && key == ' ') {
            char *pieces_p = NULL; if (asprintf(&pieces_p, "%s/projects/%s/pieces", project_root, current_project) != -1) {
                DIR *dir = opendir(pieces_p); 
                int found = 0;
                if (dir) {
                    struct dirent *entry; while ((entry = readdir(dir)) != NULL) {
                        if (entry->d_name[0] == '.' || strcmp(entry->d_name, "selector") == 0) continue;
                        int tx = get_state_int_fast(entry->d_name, "pos_x");
                        int ty = get_state_int_fast(entry->d_name, "pos_y");
                        if (tx == cursor_x && ty == cursor_y) { 
                            strncpy(active_target_id, entry->d_name, 63); 
                            char *m = NULL; if (asprintf(&m, "Selected %s", entry->d_name) != -1) { set_response(m); free(m); } 
                            found = 1;
                            break; 
                        }
                    }
                    closedir(dir);
                }
                if (!found) set_response("No piece at cursor");
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
                char *c = NULL; if (asprintf(&c, "%s/pieces/master_ledger/plugins/+x/piece_manager.+x selector set-state pos_x %d > /dev/null 2>&1", project_root, px) != -1) { run_command(c); free(c); }
                if (asprintf(&c, "%s/pieces/master_ledger/plugins/+x/piece_manager.+x selector set-state pos_y %d > /dev/null 2>&1", project_root, py) != -1) { run_command(c); free(c); }
                cursor_x = px; cursor_y = py;
            }
        }
    }
    else if (gui_focus_index == 2) { 
        if (key == 1000 || key == 'a') { map_idx--; if (map_idx < 0) map_idx = map_count - 1; } 
        else if (key == 1001 || key == 'd') { map_idx++; if (map_idx >= map_count) map_idx = 0; } 
        else if (key == 10 || key == 13) {
            if (map_idx == 0) { 
                create_new_map(); 
                stage_map_idx = 1;
                sync_current_map();
                set_response("New Map Created"); 
            } else { 
                stage_map_idx = map_idx;
                sync_current_map();
                set_response("Map Switched"); 
            }
        }
    }
    else if (gui_focus_index == 3) { 
        if (key == 1000 || key == 'a') { glyph_idx--; if (glyph_idx < 0) glyph_idx = 9; } 
        else if (key == 1001 || key == 'd') { glyph_idx++; if (glyph_idx >= 10) glyph_idx = 0; } 
        else if (key == 10 || key == 13) set_response("Glyph Armed");
    }

    if ((key == 10 || key == 13) && gui_focus_index == 1) {
        if (stage_map_idx < 1 || stage_map_idx >= map_count) {
            set_response("Select a map first!");
            return;
        }
        const char **active_set = emoji_mode ? emoji_glyphs : ascii_glyphs; const char *glyph = active_set[glyph_idx];
        int is_entity = (strcmp(glyph, "@") == 0 || strcmp(glyph, "&") == 0 || strcmp(glyph, "Z") == 0 || strcmp(glyph, "T") == 0);
        char *cmd = NULL;
        if (is_entity)
            asprintf(&cmd, "%s/pieces/apps/playrm/ops/+x/create_piece.+x '%s' %d %d %s > /dev/null 2>&1",
                     project_root, glyph, cursor_x, cursor_y, current_project);
        else
            asprintf(&cmd, "%s/pieces/apps/editor/ops/+x/place_tile.+x %s %d %d '%s' > /dev/null 2>&1",
                     project_root, project_maps[stage_map_idx], cursor_x, cursor_y, glyph);
        if (cmd) { run_command(cmd); free(cmd); if (is_entity) set_response("Piece created"); else set_response("Tile placed"); }
    }
}

int main() {
    /* CPU-SAFE: Register signal handlers */
    signal(SIGINT, handle_sigint);
    signal(SIGTERM, handle_sigint);
    
    /* CPU-SAFE: Set process group */
    setpgid(0, 0);
    
    resolve_paths();
    
    char path[MAX_CMD];
    snprintf(path, sizeof(path), "%s/pieces/apps/player_app/manager/state.txt", project_root);
    FILE *f_init = fopen(path, "r");
    if (f_init) {
        char line[MAX_LINE];
        while (fgets(line, sizeof(line), f_init)) {
            char *eq = strchr(line, '=');
            if (eq) { *eq = '\0'; char *k = trim_str(line), *v = trim_str(eq + 1); if (strcmp(k, "project_id") == 0) strncpy(current_project, v, sizeof(current_project)-1); }
        }
        fclose(f_init);
    }
    
    scan_projects(); scan_maps();
    if (map_count > 1) { map_idx = 1; stage_map_idx = 1; sync_current_map(); } else { map_idx = 0; stage_map_idx = 0; }
    write_editor_state(); trigger_render(); hit_frame_marker();
    long last_pos = 0; struct stat st; char *hist_path = NULL;
    if (asprintf(&hist_path, "%s/pieces/apps/player_app/history.txt", project_root) == -1) return 1;
    while (!g_shutdown) {
        if (!is_active_layout()) { usleep(100000); continue; }
        sync_focus();
        if (stat(hist_path, &st) == 0) {
            if (st.st_size > last_pos) {
                FILE *hf = fopen(hist_path, "r"); if (hf) {
                    fseek(hf, last_pos, SEEK_SET); int key, processed = 0;
                    while (fscanf(hf, "%d", &key) == 1) { process_key(key); processed = 1; }
                    if (processed) { write_editor_state(); trigger_render(); hit_frame_marker(); }
                    last_pos = ftell(hf); fclose(hf);
                }
            } else if (st.st_size < last_pos) last_pos = 0;
        }
        usleep(16667);
    }
    free(hist_path); return 0;
}
