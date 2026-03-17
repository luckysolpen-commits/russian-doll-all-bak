/*
 * Op: render_map
 * Usage: ./+x/render_map.+x
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
#include <ctype.h>

#define MAP_ROWS 10
#define MAP_COLS 20
#define MAX_ENTITIES 100
#define MAX_PATH 4096
#define MAX_CMD 16384
#define MAX_LINE 1024

typedef struct {
    char id[64];
    int x, y, z;
    char icon[8]; /* UTF-8 string */
} Entity;

char terrain[MAP_ROWS][MAP_COLS][8]; /* Grid of UTF-8 strings */
char project_root[MAX_PATH] = ".";
char current_project[MAX_LINE] = "template";
char app_title[256] = "PIECEMARK PLAYER";
char active_map_file[MAX_LINE] = "map_01.txt";
int current_z = 0;
int map_offset_x = 0;
int map_offset_y = 0;

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
                if (strcmp(k, "project_root") == 0) strncpy(project_root, v, MAX_PATH-1);
            }
        }
        fclose(kvp);
    }

    /* 1. Get Project ID from global manager state first */
    char *p_mgr_path = NULL;
    if (asprintf(&p_mgr_path, "%s/pieces/apps/player_app/manager/state.txt", project_root) != -1) {
        FILE *f = fopen(p_mgr_path, "r");
        if (f) {
            char line[MAX_LINE];
            while (fgets(line, sizeof(line), f)) {
                char *eq = strchr(line, '=');
                if (eq) {
                    *eq = '\0';
                    if (strcmp(trim_str(line), "project_id") == 0) strncpy(current_project, trim_str(eq+1), sizeof(current_project)-1);
                }
            }
            fclose(f);
        }
        free(p_mgr_path);
    }

    /* 2. Load Project Defaults (PDL) */
    char *pdl_path = NULL;
    if (asprintf(&pdl_path, "%s/projects/%s/project.pdl", project_root, current_project) != -1) {
        FILE *pf = fopen(pdl_path, "r");
        if (pf) {
            char line[MAX_LINE];
            while (fgets(line, sizeof(line), pf)) {
                if (strstr(line, "starting_map")) {
                    char *pipe = strrchr(line, '|');
                    if (pipe) snprintf(active_map_file, sizeof(active_map_file), "%s.txt", trim_str(pipe + 1));
                }
                else if (strstr(line, "title")) {
                    char *pipe = strrchr(line, '|');
                    if (pipe) strncpy(app_title, trim_str(pipe + 1), 255);
                }
            }
            fclose(pf);
        }
        free(pdl_path);
    }

    /* 3. FINAL OVERRIDE: Active session state from Manager (includes scrolling and specific map) */
    if (asprintf(&p_mgr_path, "%s/pieces/apps/player_app/manager/state.txt", project_root) != -1) {
        FILE *f = fopen(p_mgr_path, "r");
        if (f) {
            char line[MAX_LINE];
            while (fgets(line, sizeof(line), f)) {
                char *eq = strchr(line, '=');
                if (eq) {
                    *eq = '\0';
                    char *k = trim_str(line), *v = trim_str(eq + 1);
                    if (strcmp(k, "map_offset_x") == 0) map_offset_x = atoi(v);
                    else if (strcmp(k, "map_offset_y") == 0) map_offset_y = atoi(v);
                    else if (strcmp(k, "current_map") == 0) strncpy(active_map_file, v, sizeof(active_map_file)-1);
                }
            }
            fclose(f);
        }
        free(p_mgr_path);
    }
    
    /* Z-LEVEL: Read pos_z directly from selector state (like fuzzpet_app) */
    char *sel_path = NULL;
    if (asprintf(&sel_path, "%s/projects/%s/pieces/selector/state.txt", project_root, current_project) != -1) {
        FILE *f = fopen(sel_path, "r");
        if (f) {
            char line[MAX_LINE];
            while (fgets(line, sizeof(line), f)) {
                char *eq = strchr(line, '=');
                if (eq) {
                    *eq = '\0';
                    if (strcmp(trim_str(line), "pos_z") == 0) { current_z = atoi(trim_str(eq + 1)); break; }
                }
            }
            fclose(f);
        }
        free(sel_path);
    }
    if (current_z < 0) current_z = 0;
}

void get_piece_state(const char* piece_id, const char* key, char* out_val) {
    strcpy(out_val, "0");
    if (!piece_id || !key) return;
    char *path = NULL;
    if (asprintf(&path, "%s/projects/%s/pieces/%s/state.txt", project_root, current_project, piece_id) == -1) return;
    if (access(path, F_OK) != 0) {
        free(path);
        if (asprintf(&path, "%s/pieces/world/map_01/%s/state.txt", project_root, piece_id) == -1) return;
    }
    FILE *f = fopen(path, "r");
    if (!f) { free(path); return; }
    char line[MAX_LINE];
    while (fgets(line, sizeof(line), f)) {
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        if (strcmp(trim_str(line), key) == 0) { strcpy(out_val, trim_str(eq + 1)); break; }
    }
    fclose(f); free(path);
}

void set_state_field(const char* key, const char* val) {
    char *path = NULL;
    if (asprintf(&path, "%s/pieces/apps/player_app/state.txt", project_root) == -1) return;
    char lines[200][MAX_LINE];
    int line_count = 0, found = 0;
    FILE *f = fopen(path, "r");
    if (f) {
        while (fgets(lines[line_count], sizeof(lines[0]), f) && line_count < 199) {
            char *eq = strchr(lines[line_count], '=');
            if (eq) {
                *eq = '\0';
                if (strcmp(trim_str(lines[line_count]), key) == 0) {
                    snprintf(lines[line_count], sizeof(lines[0]), "%s=%s\n", key, val);
                    found = 1;
                } else *eq = '=';
            }
            line_count++;
        }
        fclose(f);
    }
    if (!found && line_count < 200) snprintf(lines[line_count++], sizeof(lines[0]), "%s=%s\n", key, val);
    f = fopen(path, "w");
    if (f) { for (int i = 0; i < line_count; i++) fputs(lines[i], f); fclose(f); }
    free(path);
}

int main() {
    resolve_paths();
    for (int y = 0; y < MAP_ROWS; y++) { for (int x = 0; x < MAP_COLS; x++) strcpy(terrain[y][x], "."); }
    char *map_path = NULL; FILE *mf = NULL;
    
    /* Z-LEVEL SUPPORT: Load map_{base}_z{current_z}.txt */
    char map_with_z[MAX_LINE];
    char *base = strdup(active_map_file);
    /* Remove existing _zN suffix if present */
    char *underscore_z = strstr(base, "_z");
    if (underscore_z) *underscore_z = '\0';
    snprintf(map_with_z, sizeof(map_with_z), "%s_z%d.txt", base, current_z);
    free(base);
    
    if (asprintf(&map_path, "%s/projects/%s/maps/%s", project_root, current_project, map_with_z) != -1) {
        mf = fopen(map_path, "r");
        if (!mf) { free(map_path); map_path = NULL; }
    }
    /* Fallback to original map name */
    if (!mf && asprintf(&map_path, "%s/projects/%s/maps/%s", project_root, current_project, active_map_file) != -1) {
        mf = fopen(map_path, "r");
        if (!mf) { free(map_path); map_path = NULL; }
    }
    
    if (mf) {
        char line[MAX_LINE];
        for (int i = 0; i < map_offset_y && fgets(line, sizeof(line), mf); i++);
        for (int y = 0; y < MAP_ROWS && fgets(line, sizeof(line), mf); y++) {
            char *p = line; int skip_x = map_offset_x;
            while (skip_x > 0 && *p && *p != '\n' && *p != '\r') {
                int len = ((*p & 0x80) == 0) ? 1 : ((*p & 0xE0) == 0xC0) ? 2 : ((*p & 0xF0) == 0xE0) ? 3 : 4;
                p += len; skip_x--;
            }
            for (int x = 0; x < MAP_COLS && *p && *p != '\n' && *p != '\r'; x++) {
                int len = ((*p & 0x80) == 0) ? 1 : ((*p & 0xE0) == 0xC0) ? 2 : ((*p & 0xF0) == 0xE0) ? 3 : 4;
                strncpy(terrain[y][x], p, len); terrain[y][x][len] = '\0'; p += len;
            }
        }
        fclose(mf);
    }
    if (map_path) free(map_path);
    
    Entity entities[MAX_ENTITIES]; int entity_count = 0; int selector_idx = -1; char *pieces_dir = NULL;
    if (asprintf(&pieces_dir, "%s/projects/%s/pieces", project_root, current_project) != -1) {
        DIR *dir = opendir(pieces_dir);
        if (dir) {
            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL && entity_count < MAX_ENTITIES) {
                if (entry->d_name[0] == '.') continue;
                char val[64]; get_piece_state(entry->d_name, "on_map", val);
                if (atoi(val) != 1 && strcmp(entry->d_name, "selector") != 0) continue;
                get_piece_state(entry->d_name, "pos_z", val); if (atoi(val) != current_z) continue;
                
                /* FIX: Skip map_id check for selector (it's a UI cursor, not a game piece) */
                if (strcmp(entry->d_name, "selector") != 0) {
                    char piece_map_id[MAX_LINE] = "";
                    get_piece_state(entry->d_name, "map_id", piece_map_id);
                    if (strlen(piece_map_id) > 0 && strcmp(piece_map_id, active_map_file) != 0) {
                        continue;  /* Piece belongs to different map */
                    }
                }
                
                int ex, ey; get_piece_state(entry->d_name, "pos_x", val); ex = atoi(val); get_piece_state(entry->d_name, "pos_y", val); ey = atoi(val);
                if (ex >= map_offset_x && ex < map_offset_x + MAP_COLS && ey >= map_offset_y && ey < map_offset_y + MAP_ROWS) {
                    /* Store selector for last, so it renders behind other entities */
                    if (strcmp(entry->d_name, "selector") == 0) {
                        selector_idx = entity_count;
                    }
                    strncpy(entities[entity_count].id, entry->d_name, 63);
                    entities[entity_count].x = ex - map_offset_x;
                    entities[entity_count].y = ey - map_offset_y;
                    get_piece_state(entry->d_name, "type", val);
                    if (strcmp(val, "player") == 0 || strcmp(val, "pet") == 0) strcpy(entities[entity_count].icon, "@");
                    else if (strcmp(val, "selector") == 0) strcpy(entities[entity_count].icon, "X");
                    else if (strcmp(val, "npc") == 0) strcpy(entities[entity_count].icon, "&");
                    else if (strcmp(val, "chest") == 0) strcpy(entities[entity_count].icon, "T");
                    else if (strcmp(val, "zombie") == 0) strcpy(entities[entity_count].icon, "Z");
                    else strcpy(entities[entity_count].icon, "?");
                    entity_count++;
                }
            }
            closedir(dir);
        }
        free(pieces_dir);
    }
    
    /* Move selector to end of entity list (renders behind other entities) */
    if (selector_idx >= 0 && selector_idx < entity_count - 1) {
        Entity temp = entities[selector_idx];
        for (int i = selector_idx; i < entity_count - 1; i++) {
            entities[i] = entities[i + 1];
        }
        entities[entity_count - 1] = temp;
    }
    
    /* CPU-SAFE: Use asprintf for path construction to avoid truncation warnings */
    char *view_path = NULL;
    if (asprintf(&view_path, "%s/pieces/apps/player_app/view.txt", project_root) == -1) return 1;
    char *tmp_path = NULL;
    if (asprintf(&tmp_path, "%s.tmp", view_path) == -1) { free(view_path); return 1; }
    
    /* Build new view content in memory first */
    char new_view[4096] = "";
    char line_buf[256];
    
    /* Header with Z-level */
    snprintf(line_buf, sizeof(line_buf), "╠═══════════════════════════════════════════════════════════╣\n");
    strcat(new_view, line_buf);
    snprintf(line_buf, sizeof(line_buf), "║ [Z-LEVEL]: %-3d                                            ║\n", current_z);
    strcat(new_view, line_buf);
    snprintf(line_buf, sizeof(line_buf), "╠═══════════════════════════════════════════════════════════╣\n");
    strcat(new_view, line_buf);
    
    for (int y = 0; y < MAP_ROWS; y++) {
        snprintf(line_buf, sizeof(line_buf), "║  ");
        for (int x = 0; x < MAP_COLS; x++) {
            int found = -1;
            int selector_found = -1;
            /* Find entity at this position, prefer non-selector */
            for (int i = 0; i < entity_count; i++) {
                if (entities[i].x == x && entities[i].y == y) {
                    if (strcmp(entities[i].id, "selector") == 0) {
                        selector_found = i;
                    } else {
                        found = i;
                        break;  /* Found non-selector, use it */
                    }
                }
            }
            /* If only selector at this position, use it */
            if (found == -1 && selector_found >= 0) found = selector_found;
            
            if (found != -1) snprintf(line_buf + strlen(line_buf), sizeof(line_buf) - strlen(line_buf), "%s", entities[found].icon);
            else snprintf(line_buf + strlen(line_buf), sizeof(line_buf) - strlen(line_buf), "%s", terrain[y][x]);
        }
        snprintf(line_buf + strlen(line_buf), sizeof(line_buf) - strlen(line_buf), "                                     ║\n");
        strcat(new_view, line_buf);
    }
    
    /* DEDUPLICATION: Only write if view changed */
    int should_write = 1;
    FILE *old_view = fopen(view_path, "r");
    if (old_view) {
        char old_view_content[4096] = "";
        char old_line[256];
        while (fgets(old_line, sizeof(old_line), old_view)) {
            strcat(old_view_content, old_line);
        }
        fclose(old_view);
        if (strcmp(old_view_content, new_view) == 0) {
            should_write = 0;  /* No change, skip write */
        }
    }
    
    if (should_write) {
        FILE *vp = fopen(tmp_path, "w");
        if (vp) {
            fputs(new_view, vp);
            fclose(vp);
            rename(tmp_path, view_path);
        }
    }
    free(view_path);
    free(tmp_path);
    
    set_state_field("project_id", current_project); set_state_field("app_title", app_title);
    return 0;
}
