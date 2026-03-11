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
#define MAX_LINE 1024

typedef struct {
    char id[64];
    int x, y, z;
    char icon[8]; /* UTF-8 string */
} Entity;

char terrain[MAP_ROWS][MAP_COLS][8]; /* Grid of UTF-8 strings */
char project_root[MAX_PATH] = ".";
char current_project[MAX_LINE] = "template";
char active_map_file[MAX_LINE] = "map_0001_z0.txt";
int current_z = 0;

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
    
    /* Load Player Manager State */
    char *p_mgr_path = NULL;
    if (asprintf(&p_mgr_path, "%s/pieces/apps/player_app/manager/state.txt", project_root) != -1) {
        FILE *f = fopen(p_mgr_path, "r");
        if (f) {
            char line[MAX_LINE];
            while (fgets(line, sizeof(line), f)) {
                char *eq = strchr(line, '=');
                if (eq) {
                    *eq = '\0';
                    char *k = trim_str(line);
                    char *v = trim_str(eq + 1);
                    if (strcmp(k, "project_id") == 0) strncpy(current_project, v, sizeof(current_project)-1);
                    else if (strcmp(k, "current_z") == 0) current_z = atoi(v);
                }
            }
            fclose(f);
        }
        free(p_mgr_path);
    }

    /* Load Editor State for map selection */
    char *e_state_path = NULL;
    if (asprintf(&e_state_path, "%s/pieces/apps/editor/state.txt", project_root) != -1) {
        FILE *f = fopen(e_state_path, "r");
        if (f) {
            char line[MAX_LINE];
            while (fgets(line, sizeof(line), f)) {
                char *eq = strchr(line, '=');
                if (eq && strncmp(trim_str(line), "map_palette_view", 16) == 0) {
                    char *v = trim_str(eq + 1);
                    char *start = strchr(v, '[');
                    char *end = strchr(v, ']');
                    if (start && end && end > start) {
                        int len = end - start - 1;
                        strncpy(active_map_file, start + 1, len);
                        active_map_file[len] = '\0';
                    }
                }
            }
            fclose(f);
        }
        free(e_state_path);
    }
}

void get_piece_state(const char* piece_id, const char* key, char* out_val) {
    char *path = NULL;
    if (asprintf(&path, "%s/projects/%s/pieces/%s/state.txt", project_root, current_project, piece_id) == -1) {
        strcpy(out_val, "unknown");
        return;
    }
    FILE *f = fopen(path, "r");
    if (f) {
        char line[MAX_LINE];
        while (fgets(line, sizeof(line), f)) {
            char *eq = strchr(line, '=');
            if (eq) {
                *eq = '\0';
                if (strcmp(trim_str(line), key) == 0) {
                    strncpy(out_val, trim_str(eq + 1), 63);
                    fclose(f); free(path); return;
                }
            }
        }
        fclose(f);
    }
    free(path);
    strcpy(out_val, "unknown");
}

int main() {
    resolve_paths();
    
    /* 1. Load terrain (UTF-8 safe) */
    for (int y = 0; y < MAP_ROWS; y++) {
        for (int x = 0; x < MAP_COLS; x++) strcpy(terrain[y][x], ".");
    }

    char *map_path = NULL;
    if (asprintf(&map_path, "%s/projects/%s/maps/%s", project_root, current_project, active_map_file) != -1) {
        FILE *mf = fopen(map_path, "r");
        if (mf) {
            char line[MAX_LINE];
            for (int y = 0; y < MAP_ROWS && fgets(line, sizeof(line), mf); y++) {
                char *p = line;
                for (int x = 0; x < MAP_COLS && *p && *p != '\n' && *p != '\r'; x++) {
                    int len = 1;
                    if ((*p & 0x80) == 0) len = 1;
                    else if ((*p & 0xE0) == 0xC0) len = 2;
                    else if ((*p & 0xF0) == 0xE0) len = 3;
                    else if ((*p & 0xF8) == 0xF0) len = 4;
                    strncpy(terrain[y][x], p, len);
                    terrain[y][x][len] = '\0';
                    p += len;
                }
            }
            fclose(mf);
        }
        free(map_path);
    }
    
    /* 2. Load entities */
    Entity entities[MAX_ENTITIES];
    int entity_count = 0;
    char *pieces_dir = NULL;
    if (asprintf(&pieces_dir, "%s/projects/%s/pieces", project_root, current_project) != -1) {
        DIR *dir = opendir(pieces_dir);
        if (dir) {
            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL && entity_count < MAX_ENTITIES) {
                if (entry->d_name[0] == '.') continue;
                char val[64];
                get_piece_state(entry->d_name, "on_map", val);
                if (strcmp(val, "1") != 0) continue;
                get_piece_state(entry->d_name, "pos_z", val);
                if (atoi(val) != current_z) continue;

                strncpy(entities[entity_count].id, entry->d_name, 63);
                get_piece_state(entry->d_name, "pos_x", val);
                entities[entity_count].x = atoi(val);
                get_piece_state(entry->d_name, "pos_y", val);
                entities[entity_count].y = atoi(val);
                
                get_piece_state(entry->d_name, "type", val);
                if (strcmp(val, "player") == 0) strcpy(entities[entity_count].icon, "@");
                else if (strcmp(val, "selector") == 0) strcpy(entities[entity_count].icon, "X");
                else if (strcmp(val, "npc") == 0) strcpy(entities[entity_count].icon, "&");
                else if (strcmp(val, "chest") == 0) strcpy(entities[entity_count].icon, "T");
                else strcpy(entities[entity_count].icon, "?");
                
                entity_count++;
            }
            closedir(dir);
        }
        free(pieces_dir);
    }
    
    /* 3. Compose View */
    char *view_path = NULL;
    if (asprintf(&view_path, "%s/pieces/apps/player_app/view.txt", project_root) != -1) {
        char *tmp_view = NULL; asprintf(&tmp_view, "%s.tmp", view_path);
        FILE *vp = fopen(tmp_view, "w");
        if (vp) {
            for (int y = 0; y < MAP_ROWS; y++) {
                fprintf(vp, "║  ");
                for (int x = 0; x < MAP_COLS; x++) {
                    int found = -1;
                    for (int i = 0; i < entity_count; i++) {
                        if (entities[i].x == x && entities[i].y == y) {
                            if (found == -1 || entities[i].icon[0] == '@') found = i;
                        }
                    }
                    if (found != -1) fprintf(vp, "%s", entities[found].icon);
                    else fprintf(vp, "%s", terrain[y][x]);
                }
                fprintf(vp, "                                     ║\n");
            }
            fclose(vp);
            rename(tmp_view, view_path);
        }
        free(view_path); free(tmp_view);
    }
    
    return 0;
}
