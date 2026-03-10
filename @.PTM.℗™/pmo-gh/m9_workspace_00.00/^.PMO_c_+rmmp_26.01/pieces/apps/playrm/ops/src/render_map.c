/*
 * Op: render_map
 * Usage: ./+x/render_map.+x
 * 
 * Behavior:
 *   1. Reads engine state for current project_id and current_z
 *   2. Loads map terrain from projects/{project_id}/maps/map_0001_z{z}.txt
 *   3. Scans projects/{project_id}/pieces/ for all entities
 *   4. Composes viewport with terrain + entity icons (Hero @, NPC &, Chest T, etc.)
 *   5. Writes output to pieces/apps/player_app/view.txt
 *   6. Signals frame_changed.txt
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
#define MAX_LINE 256

typedef struct {
    char id[64];
    int x, y, z;
    char icon;
} Entity;

char terrain[MAP_ROWS][MAP_COLS + 1];
char project_root[MAX_PATH] = ".";
char current_project[MAX_LINE] = "template";
int current_z = 0;

char* trim_str(char *str) {
    char *end;
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
    
    char engine_state_path[MAX_PATH];
    snprintf(engine_state_path, sizeof(engine_state_path), "%s/pieces/apps/player_app/manager/state.txt", project_root);
    FILE *ef = fopen(engine_state_path, "r");
    if (ef) {
        char line[MAX_LINE];
        while (fgets(line, sizeof(line), ef)) {
            char *eq = strchr(line, '=');
            if (eq) {
                *eq = '\0';
                char *k = trim_str(line);
                char *v = trim_str(eq + 1);
                if (strcmp(k, "project_id") == 0) strncpy(current_project, v, sizeof(current_project)-1);
                else if (strcmp(k, "current_z") == 0) current_z = atoi(v);
            }
        }
        fclose(ef);
    }
}

void get_piece_state(const char* piece_id, const char* key, char* out_val) {
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s/projects/%s/pieces/%s/state.txt", project_root, current_project, piece_id);
    FILE *f = fopen(path, "r");
    if (f) {
        char line[MAX_LINE];
        while (fgets(line, sizeof(line), f)) {
            char *eq = strchr(line, '=');
            if (eq) {
                *eq = '\0';
                if (strcmp(trim_str(line), key) == 0) {
                    strncpy(out_val, trim_str(eq + 1), 63);
                    fclose(f); return;
                }
            }
        }
        fclose(f);
    }
    strcpy(out_val, "unknown");
}

int main() {
    resolve_paths();
    
    /* 1. Load terrain */
    char map_path[MAX_PATH];
    snprintf(map_path, sizeof(map_path), "%s/projects/%s/maps/map_0001_z%d.txt", project_root, current_project, current_z);
    
    FILE *mf = fopen(map_path, "r");
    if (mf) {
        for (int y = 0; y < MAP_ROWS; y++) {
            if (fgets(terrain[y], MAP_COLS + 2, mf)) {
                terrain[y][strcspn(terrain[y], "\n\r")] = 0;
            } else {
                memset(terrain[y], '.', MAP_COLS);
                terrain[y][MAP_COLS] = '\0';
            }
        }
        fclose(mf);
    } else {
        for (int y = 0; y < MAP_ROWS; y++) {
            memset(terrain[y], '.', MAP_COLS);
            terrain[y][MAP_COLS] = '\0';
        }
    }
    
    /* 2. Load entities */
    Entity entities[MAX_ENTITIES];
    int entity_count = 0;
    char pieces_dir[MAX_PATH];
    snprintf(pieces_dir, MAX_PATH, "%s/projects/%s/pieces", project_root, current_project);
    
    DIR *dir = opendir(pieces_dir);
    if (dir) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL && entity_count < MAX_ENTITIES) {
            if (entry->d_name[0] == '.') continue;
            
            char val[64];
            get_piece_state(entry->d_name, "on_map", val);
            if (strcmp(val, "1") != 0) continue;

            get_piece_state(entry->d_name, "pos_z", val);
            int ez = (strcmp(val, "unknown") == 0) ? 0 : atoi(val);
            
            if (ez == current_z) {
                strncpy(entities[entity_count].id, entry->d_name, 63);
                get_piece_state(entry->d_name, "pos_x", val);
                entities[entity_count].x = atoi(val);
                get_piece_state(entry->d_name, "pos_y", val);
                entities[entity_count].y = atoi(val);
                
                /* Determine icon based on type or name */
                get_piece_state(entry->d_name, "type", val);
                if (strcmp(val, "player") == 0) entities[entity_count].icon = '@';
                else if (strcmp(val, "selector") == 0) entities[entity_count].icon = 'X';
                else if (strcmp(val, "npc") == 0) entities[entity_count].icon = '&';
                else if (strcmp(val, "chest") == 0) entities[entity_count].icon = 'T';
                else entities[entity_count].icon = '?';
                
                entity_count++;
            }
        }
        closedir(dir);
    }
    
    /* 3. Compose View */
    char view_path[MAX_PATH];
    snprintf(view_path, sizeof(view_path), "%s/pieces/apps/player_app/view.txt", project_root);
    FILE *vp = fopen(view_path, "w");
    if (!vp) return 1;
    
    for (int y = 0; y < MAP_ROWS; y++) {
        fprintf(vp, "║  ");
        for (int x = 0; x < MAP_COLS; x++) {
            int found = -1;
            /* Priority: Player > NPC > Selector > Terrain */
            for (int i = 0; i < entity_count; i++) {
                if (entities[i].x == x && entities[i].y == y) {
                    if (found == -1) found = i;
                    else if (entities[i].icon == '@') found = i;
                    else if (entities[i].icon == '&' && entities[found].icon != '@') found = i;
                }
            }
            
            if (found != -1) fprintf(vp, "%c", entities[found].icon);
            else fprintf(vp, "%c", terrain[y][x]);
        }
        fprintf(vp, "                                     ║\n");
    }
    fclose(vp);
    
    /* 4. Signal Change */
    char signal_path[MAX_PATH];
    snprintf(signal_path, sizeof(signal_path), "%s/pieces/display/frame_changed.txt", project_root);
    FILE *sf = fopen(signal_path, "a");
    if (sf) { fprintf(sf, "V\n"); fclose(sf); }
    
    return 0;
}
