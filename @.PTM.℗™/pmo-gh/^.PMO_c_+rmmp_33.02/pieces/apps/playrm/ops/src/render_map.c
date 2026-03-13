/*
 * Op: render_map
 * Usage: ./+x/render_map.+x
 * 
 * Enhanced renderer for Player V2:
 * 1. Dynamic path construction (asprintf) - silenced warnings
 * 2. Unified 10x20 rendering
 * 3. Fuzzpet-specific stats extraction
 * 4. Response extraction from master_ledger
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
                if (strcmp(k, "project_root") == 0) strncpy(project_root, v, MAX_PATH-1);
            }
        }
        fclose(kvp);
    }

    /* PRIMARY: Read project_id from global manager state */
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
    
    /* Load Project Meta for map name and title */
    char *pdl_path = NULL;
    if (asprintf(&pdl_path, "%s/projects/%s/project.pdl", project_root, current_project) != -1) {
        FILE *pf = fopen(pdl_path, "r");
        if (pf) {
            char line[MAX_LINE];
            while (fgets(line, sizeof(line), pf)) {
                if (strstr(line, "starting_map")) {
                    char *pipe = strrchr(line, '|');
                    if (pipe) {
                        char *v = trim_str(pipe + 1);
                        snprintf(active_map_file, sizeof(active_map_file), "%s.txt", v);
                    }
                }
                else if (strstr(line, "title")) {
                    char *pipe = strrchr(line, '|');
                    if (pipe) {
                        char *v = trim_str(pipe + 1);
                        strncpy(app_title, v, 255);
                    }
                }
            }
            fclose(pf);
        }
        free(pdl_path);
    }
}

/* FIX: Robust get_piece_state with error handling */
void get_piece_state(const char* piece_id, const char* key, char* out_val) {
    strcpy(out_val, "0");  /* Default value */
    if (!piece_id || !key) return;
    
    char *path = NULL;
    // Try projects pieces first
    if (asprintf(&path, "%s/projects/%s/pieces/%s/state.txt", project_root, current_project, piece_id) == -1) return;
    
    if (access(path, F_OK) != 0) {
        free(path);
        // Fallback to global world pieces
        if (asprintf(&path, "%s/pieces/world/map_01/%s/state.txt", project_root, piece_id) == -1) return;
    }
    
    FILE *f = fopen(path, "r");
    if (!f) {
        free(path);
        return;
    }
    
    char line[MAX_LINE];
    while (fgets(line, sizeof(line), f)) {
        char *eq = strchr(line, '=');
        if (!eq) continue;
        
        *eq = '\0';
        char *k = trim_str(line);
        char *v = trim_str(eq + 1);
        
        if (strcmp(k, key) == 0) {
            if (strlen(v) > 0) strcpy(out_val, v);
            break;
        }
    }
    fclose(f);
    free(path);
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
    
    if (!found && line_count < 200) {
        snprintf(lines[line_count++], sizeof(lines[0]), "%s=%s\n", key, val);
    }
    
    f = fopen(path, "w");
    if (f) {
        for (int i = 0; i < line_count; i++) fputs(lines[i], f);
        fclose(f);
    }
    free(path);
}

int main() {
    resolve_paths();
    
    /* 1. Load Terrain */
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
                int on_map_val = atoi(val);
                if (on_map_val == 0) {
                    if (strcmp(entry->d_name, "selector") == 0 || 
                        strcmp(entry->d_name, "fuzzpet") == 0 ||
                        strcmp(entry->d_name, "hero") == 0 ||
                        strncmp(entry->d_name, "pet_", 4) == 0 ||
                        strncmp(entry->d_name, "zombie_", 7) == 0) on_map_val = 1;
                }
                if (on_map_val != 1) continue;
                
                get_piece_state(entry->d_name, "pos_z", val);
                if (atoi(val) != current_z) continue;

                strncpy(entities[entity_count].id, entry->d_name, 63);
                entities[entity_count].id[63] = '\0';
                get_piece_state(entry->d_name, "pos_x", val);
                entities[entity_count].x = atoi(val);
                get_piece_state(entry->d_name, "pos_y", val);
                entities[entity_count].y = atoi(val);

                get_piece_state(entry->d_name, "type", val);
                if (strcmp(val, "player") == 0 || strcmp(val, "pet") == 0) strcpy(entities[entity_count].icon, "@");
                else if (strcmp(val, "selector") == 0) strcpy(entities[entity_count].icon, "X");
                else if (strcmp(val, "npc") == 0) strcpy(entities[entity_count].icon, "&");
                else if (strcmp(val, "chest") == 0) strcpy(entities[entity_count].icon, "T");
                else if (strcmp(val, "zombie") == 0) strcpy(entities[entity_count].icon, "Z");
                else strcpy(entities[entity_count].icon, "?");
                entity_count++;
            }
            closedir(dir);
        }
        free(pieces_dir);
    }
    
    /* 3. Extract Stats */
    char pet_status[256] = "";
    char p_name[64] = "FuzzPet", p_h[16] = "0", p_e[16] = "0", p_l[16] = "0", p_hp[16] = "0";
    int emoji_mode = 0;
    if (strcmp(current_project, "fuzzpet_v2") == 0) {
        char em[16];
        get_piece_state("fuzzpet", "hunger", p_h);
        get_piece_state("fuzzpet", "energy", p_e);
        get_piece_state("fuzzpet", "level", p_l);
        get_piece_state("fuzzpet", "happiness", p_hp);
        get_piece_state("fuzzpet", "emoji_mode", em);
        get_piece_state("fuzzpet", "name", p_name);
        if (strlen(p_name) == 0 || strcmp(p_name, "0") == 0) strcpy(p_name, "FuzzPet");
        emoji_mode = atoi(em);
        snprintf(pet_status, sizeof(pet_status), "[%s]: HGR:%s | HAP:%s | ENR:%s | LVL:%s", p_name, p_h, p_hp, p_e, p_l);
    }

    if (emoji_mode) {
        for (int y = 0; y < MAP_ROWS; y++) {
            for (int x = 0; x < MAP_COLS; x++) {
                if (strcmp(terrain[y][x], "#") == 0) strcpy(terrain[y][x], "🧱");
                else if (strcmp(terrain[y][x], ".") == 0) strcpy(terrain[y][x], "🟩");
                else if (strcmp(terrain[y][x], "R") == 0) strcpy(terrain[y][x], "🌲");
                else if (strcmp(terrain[y][x], "T") == 0) strcpy(terrain[y][x], "💰");
            }
        }
        for (int i = 0; i < entity_count; i++) {
            if (strcmp(entities[i].icon, "@") == 0) strcpy(entities[i].icon, "🐶");
            else if (strcmp(entities[i].icon, "X") == 0) strcpy(entities[i].icon, "🎯");
            else if (strcmp(entities[i].icon, "Z") == 0) strcpy(entities[i].icon, "🧟");
            else if (strcmp(entities[i].icon, "T") == 0) strcpy(entities[i].icon, "💰");
        }
    }

    /* 4. Extract Response */
    char last_resp[256] = "";
    char *ledger_p = NULL;
    if (asprintf(&ledger_p, "%s/pieces/master_ledger/master_ledger.txt", project_root) != -1) {
        FILE *lf = fopen(ledger_p, "r");
        if (lf) {
            char line[1024];
            while (fgets(line, sizeof(line), lf)) {
                char *msg_ptr = NULL;
                if ((msg_ptr = strstr(line, "Message: "))) msg_ptr += 9;
                else if ((msg_ptr = strstr(line, "EventFire: "))) msg_ptr += 11;
                else if ((msg_ptr = strstr(line, "ResponseRequest: "))) msg_ptr += 17;
                if (msg_ptr) {
                    char *pipe = strstr(msg_ptr, " |");
                    int len = pipe ? (int)(pipe - msg_ptr) : (int)strlen(msg_ptr);
                    if (len > 255) len = 255;
                    strncpy(last_resp, msg_ptr, len); last_resp[len] = '\0';
                    char *end = last_resp + strlen(last_resp) - 1;
                    while(end >= last_resp && isspace((unsigned char)*end)) { *end = '\0'; end--; }
                }
            }
            fclose(lf);
        }
        free(ledger_p);
    }

    /* 5. Load Active Target & Compose View */
    char active_target_id[64] = "selector";
    char last_key[64] = "None";
    char *mgr_path = NULL;
    if (asprintf(&mgr_path, "%s/pieces/apps/player_app/manager/state.txt", project_root) != -1) {
        FILE *mf = fopen(mgr_path, "r");
        if (mf) {
            char line[MAX_LINE];
            while (fgets(line, sizeof(line), mf)) {
                char *eq = strchr(line, '=');
                if (eq) {
                    *eq = '\0';
                    if (strcmp(trim_str(line), "active_target_id") == 0) strncpy(active_target_id, trim_str(eq + 1), 63);
                    else if (strcmp(trim_str(line), "last_key") == 0) strncpy(last_key, trim_str(eq + 1), 63);
                }
            }
            fclose(mf);
        }
        free(mgr_path);
    }

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
                            if (strcmp(entities[i].id, active_target_id) == 0) { found = i; break; }
                            if (found == -1 || entities[i].icon[0] == '@') found = i;
                        }
                    }
                    if (found != -1) fprintf(vp, "%s", entities[found].icon);
                    else fprintf(vp, "%s", terrain[y][x]);
                }
                fprintf(vp, "                                     ║\n");
            }
            fclose(vp); rename(tmp_view, view_path);
        }
        free(tmp_view); free(view_path);
    }
    
    /* 6. Non-Destructive State Update */
    set_state_field("project_id", current_project);
    set_state_field("app_title", app_title);
    set_state_field("pet_status", pet_status);
    set_state_field("pet_name", p_name);
    set_state_field("pet_hunger", p_h);
    set_state_field("pet_happiness", p_hp);
    set_state_field("pet_energy", p_e);
    set_state_field("pet_level", p_l);
    set_state_field("last_response", last_resp);
    char z_buf[16]; snprintf(z_buf, 16, "%d", current_z);
    set_state_field("current_z", z_buf);
    set_state_field("active_target_id", active_target_id);
    set_state_field("last_key", last_key);
    
    return 0;
}
