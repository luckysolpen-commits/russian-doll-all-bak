#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ctype.h>
#include <dirent.h>
#include <stdarg.h>

// TPM Map Render (v4.4 - BUFFER FIX)
// Responsibility: Draw spatial map, sync view.

#define MAP_ROWS 10
#define MAP_COLS 20
#define MAX_ENTITIES 100
#define MAX_PATH 16384

typedef struct {
    char id[64];
    int x, y;
    int on_map;
    char emoji[64];
} Entity;

char map[MAP_ROWS][MAP_COLS + 1];
char project_root[MAX_PATH] = ".";

char* trim_str(char *str) {
    char *end;
    while(isspace((unsigned char)*str)) str++;
    if(*str == 0) return str;
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

void resolve_root() {
    FILE *kvp = fopen("pieces/locations/location_kvp", "r");
    if (kvp) {
        char line[MAX_PATH];
        while (fgets(line, sizeof(line), kvp)) {
            if (strncmp(line, "project_root=", 13) == 0) {
                char *v = line + 13;
                v[strcspn(v, "\n\r")] = 0;
                snprintf(project_root, MAX_PATH, "%s", v);
                break;
            }
        }
        fclose(kvp);
    }
}

void get_state_fast(const char* piece_id, const char* key, char* out_val) {
    char *path = malloc(MAX_PATH);
    if (!path) { strcpy(out_val, "unknown"); return; }
    char line[MAX_PATH];
    snprintf(path, MAX_PATH, "./pieces/world/map_01/%s/state.txt", piece_id);
    if (access(path, F_OK) != 0) {
        if (strcmp(piece_id, "fuzzpet") == 0) snprintf(path, MAX_PATH, "./pieces/apps/fuzzpet_app/fuzzpet/state.txt");
        else if (strcmp(piece_id, "manager") == 0) snprintf(path, MAX_PATH, "./pieces/apps/fuzzpet_app/manager/state.txt");
        else if (strcmp(piece_id, "selector") == 0) snprintf(path, MAX_PATH, "./pieces/apps/fuzzpet_app/selector/state.txt");
    }
    FILE *f = fopen(path, "r");
    if (f) {
        while (fgets(line, sizeof(line), f)) {
            char *eq = strchr(line, '=');
            if (eq) {
                *eq = '\0';
                if (strcmp(trim_str(line), key) == 0) {
                    strncpy(out_val, trim_str(eq + 1), 255);
                    fclose(f); free(path); return;
                }
            }
        }
        fclose(f);
    }
    free(path);
    strcpy(out_val, "unknown");
}

int get_state_int_fast(const char* piece_id, const char* key) {
    char val[256]; get_state_fast(piece_id, key, val);
    if (strcmp(val, "unknown") == 0 || val[0] == '\0') return -1;
    return atoi(val);
}

void load_map(int z_level) {
    char path[MAX_PATH];
    snprintf(path, MAX_PATH, "./pieces/apps/fuzzpet_app/world/map_z%d.txt", z_level);
    if (access(path, F_OK) != 0) {
        snprintf(path, MAX_PATH, "./pieces/apps/fuzzpet_app/world/map.txt");
    }
    FILE *f = fopen(path, "r");
    if (f) {
        for (int y = 0; y < MAP_ROWS; y++) {
            if (fgets(map[y], MAP_COLS + 2, f)) { 
                map[y][strcspn(map[y], "\n\r")] = 0; 
                for (int x = 0; x < MAP_COLS; x++) if (map[y][x] == '@') map[y][x] = '.';
            } else map[y][0] = '\0'; 
        }
        fclose(f);
    }
}

int main() {
    int selector_z = get_state_int_fast("selector", "pos_z");
    if (selector_z == -1) selector_z = 0;
    
    load_map(selector_z);
    char active_id[64] = "unknown", last_key[64] = "None", last_resp[512] = "";
    int emoji_mode = 0;

    FILE *sf = fopen("./pieces/apps/fuzzpet_app/manager/state.txt", "r");
    if (sf) {
        char line[MAX_PATH];
        while (fgets(line, sizeof(line), sf)) {
            line[strcspn(line, "\n\r")] = 0;
            if (strncmp(line, "active_target_id=", 17) == 0) strncpy(active_id, line + 17, 63);
            if (strncmp(line, "last_key=", 9) == 0) strncpy(last_key, line + 9, 63);
        }
        fclose(sf);
    }

    FILE *mir = fopen("./pieces/apps/fuzzpet_app/fuzzpet/state.txt", "r");
    if (mir) {
        char line[MAX_PATH];
        while (fgets(line, sizeof(line), mir)) {
            if (strncmp(line, "emoji_mode=", 11) == 0) emoji_mode = atoi(line + 11);
        }
        fclose(mir);
    }

    FILE *rf = fopen("./pieces/apps/fuzzpet_app/fuzzpet/last_response.txt", "r");
    if (rf) { if (fgets(last_resp, 512, rf)) { char *tr = trim_str(last_resp); strncpy(last_resp, tr, 511); } fclose(rf); }

    Entity entities[MAX_ENTITIES];
    int entity_count = 0;
    DIR *dir = opendir("./pieces/world/map_01");
    if (dir) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL && entity_count < MAX_ENTITIES) {
            char *id = entry->d_name;
            if (id[0] == '.' || strcmp(id, "selector") == 0) continue;
            if (get_state_int_fast(id, "on_map") == 1) {
                int ent_z = get_state_int_fast(id, "pos_z");
                if (ent_z == -1) ent_z = 0;
                if (ent_z != selector_z) continue;

                strncpy(entities[entity_count].id, id, 63);
                entities[entity_count].x = get_state_int_fast(id, "pos_x");
                entities[entity_count].y = get_state_int_fast(id, "pos_y");
                get_state_fast(id, "emoji", entities[entity_count].emoji);
                entity_count++;
            }
        }
        closedir(dir);
    }

    char render_buf[MAP_ROWS][MAX_PATH]; 
    for (int y = 0; y < MAP_ROWS; y++) {
        char* dst = render_buf[y];
        for (int x = 0; x < MAP_COLS; x++) {
            char cell = map[y][x];
            int ent_found = 0;
            for (int i = 0; i < entity_count; i++) {
                if (entities[i].x == x && entities[i].y == y) {
                    if (emoji_mode) {
                        if (strcmp(entities[i].emoji, "unknown") != 0 && strlen(entities[i].emoji) > 0) strcpy(dst, entities[i].emoji);
                        else {
                            if (strcmp(entities[i].id, "pet_01") == 0) strcpy(dst, "🐶");
                            else if (strcmp(entities[i].id, "pet_02") == 0) strcpy(dst, "📦");
                            else strcpy(dst, "❓");
                        }
                    } else {
                        if (strcmp(entities[i].id, "pet_01") == 0) strcpy(dst, "@");
                        else if (strcmp(entities[i].id, "pet_02") == 0) strcpy(dst, "&");
                        else strcpy(dst, "?");
                    }
                    dst += strlen(dst); ent_found = 1; break;
                }
            }
            if (!ent_found) {
                if (strcmp(active_id, "selector") == 0 && get_state_int_fast("selector", "pos_x") == x && get_state_int_fast("selector", "pos_y") == y) {
                    if (emoji_mode) strcpy(dst, "🎯"); else strcpy(dst, "X");
                } else {
                    if (emoji_mode) {
                        if (cell == '#') strcpy(dst, "🧱");
                        else if (cell == '.') strcpy(dst, "⬛");
                        else if (cell == 'T') strcpy(dst, "🌲");
                        else if (cell == 'R') strcpy(dst, "⛰️ ");
                        else if (cell == '>') strcpy(dst, "⬆️ ");
                        else if (cell == '<') strcpy(dst, "⬇️ ");
                        else { *dst = cell; *(dst+1) = '\0'; }
                    } else { *dst = cell; *(dst+1) = '\0'; }
                }
                dst += strlen(dst);
            }
        }
        *dst = '\0';
    }

    FILE *fp = fopen("./pieces/apps/fuzzpet_app/fuzzpet/view.txt", "w");
    if (fp) {
        for (int y = 0; y < MAP_ROWS; y++) fprintf(fp, "║  %-20s                                     ║\n", render_buf[y]);
        fprintf(fp, "╠═══════════════════════════════════════════════════════════╣\n");
        fprintf(fp, "║ [RESP]: %-49s ║\n", last_resp);
        fprintf(fp, "║ [Z-LEVEL]: %-3d | [ACTIVE]: %-10s | [KEY]: %-10s ║\n", selector_z, active_id, last_key);
        fclose(fp);
    }

    FILE *marker = fopen("./pieces/apps/fuzzpet_app/fuzzpet/view_changed.txt", "a");
    if (marker) { fputc('X', marker); fputc('\n', marker); fclose(marker); }
    return 0;
}
